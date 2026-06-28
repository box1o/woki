mergeInto(LibraryManager.library, {
  $WokiExt__deps: ['$UTF8ToString'],
  $WokiExt: {
    // Keep in sync with modules/extension/sdk/woki_limits.h and sdk/perm_bits.h
    maxLogLen: 4096,
    instances: new Map(),
    lastError: '',
    ok: 0,
    err: -1,
    denied: -2,
    noSpace: -3,
    notFound: -4,
    invalid: -5,
    log: 1 << 0,
    paths: 1 << 1,
    storage: 1 << 2,
    config: 1 << 3,
    events: 1 << 4,
    setError(message) {
      this.lastError = String(message || 'unknown web extension error');
      return this.err;
    },
    has(record, permission) {
      return (record.permissions & permission) !== 0;
    },
    memory(record) {
      return record.instance.exports.memory;
    },
    bytes(record, ptr, len) {
      if (ptr < 0 || len < 0) {
        throw new Error('negative guest pointer or length');
      }
      const memory = this.memory(record);
      if (!memory) {
        throw new Error('guest module does not export memory');
      }
      const view = new Uint8Array(memory.buffer);
      if (ptr + len > view.length) {
        throw new Error('guest memory access is out of bounds');
      }
      return view.subarray(ptr, ptr + len);
    },
    text(record, ptr, len) {
      return new TextDecoder().decode(this.bytes(record, ptr, len));
    },
    cstr(record, ptr) {
      const memory = this.memory(record);
      if (!memory || ptr < 0) {
        throw new Error('invalid guest string pointer');
      }
      const view = new Uint8Array(memory.buffer);
      if (ptr >= view.length) {
        throw new Error('guest string pointer is out of bounds');
      }
      let end = ptr;
      const max = Math.min(view.length, ptr + 4097);
      while (end < max && view[end] !== 0) {
        ++end;
      }
      if (end === max) {
        throw new Error('guest string is not null terminated');
      }
      return new TextDecoder().decode(view.subarray(ptr, end));
    },
    writeText(record, ptr, cap, value) {
      if (ptr < 0 || cap <= 0) {
        return this.invalid;
      }
      const encoded = new TextEncoder().encode(String(value));
      if (encoded.length + 1 > cap) {
        return this.noSpace;
      }
      const out = this.bytes(record, ptr, cap);
      out.set(encoded, 0);
      out[encoded.length] = 0;
      return this.ok;
    },
    readU32(record, ptr) {
      const bytes = this.bytes(record, ptr, 4);
      return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
    },
    writeU32(record, ptr, value) {
      const bytes = this.bytes(record, ptr, 4);
      bytes[0] = value & 0xff;
      bytes[1] = (value >> 8) & 0xff;
      bytes[2] = (value >> 16) & 0xff;
      bytes[3] = (value >> 24) & 0xff;
    },
    ensureDir(path) {
      const parts = path.split('/').filter(Boolean);
      let current = '';
      for (const part of parts) {
        current += '/' + part;
        try {
          FS.mkdir(current);
        } catch (error) {
          if (!error || error.errno !== 20) {
            throw error;
          }
        }
      }
    },
    safePath(root, rel) {
      if (!rel || rel.startsWith('/') || rel.split('/').includes('..')) {
        return null;
      }
      return root.replace(/\/+$/, '') + '/' + rel;
    },
    imports(record) {
      const host = {};
      host.host_log = (level, ptr, len) => {
        if (!this.has(record, this.log)) {
          return this.denied;
        }
        try {
          const message = this.text(record, ptr, len > this.maxLogLen ? this.maxLogLen : len);
          const prefix = `[${record.id}]`;
          if (level === 3) console.error(prefix, message);
          else if (level === 2) console.warn(prefix, message);
          else console.log(prefix, message);
          return this.ok;
        } catch (error) {
          return this.invalid;
        }
      };
      host.host_path_data = (outPtr, outCap) => {
        if (!this.has(record, this.paths)) return this.denied;
        try {
          this.ensureDir(record.dataPath);
          return this.writeText(record, outPtr, outCap, record.dataPath);
        } catch (error) {
          return this.setError(error.message);
        }
      };
      host.host_path_cache = (outPtr, outCap) => {
        if (!this.has(record, this.paths)) return this.denied;
        try {
          this.ensureDir(record.cachePath);
          return this.writeText(record, outPtr, outCap, record.cachePath);
        } catch (error) {
          return this.setError(error.message);
        }
      };
      const readFile = (path, outPtr, inoutLenPtr) => {
        if (!this.has(record, this.storage)) return this.denied;
        const fullPath = this.safePath(record.dataPath, path);
        if (!fullPath) return this.invalid;
        try {
          const data = FS.readFile(fullPath);
          const cap = this.readU32(record, inoutLenPtr);
          this.writeU32(record, inoutLenPtr, data.length);
          if (cap < data.length) return this.noSpace;
          this.bytes(record, outPtr, cap).set(data, 0);
          return this.ok;
        } catch (error) {
          return this.notFound;
        }
      };
      host.host_file_read = (pathPtr, outPtr, inoutLenPtr) => {
        try {
          return readFile(this.cstr(record, pathPtr), outPtr, inoutLenPtr);
        } catch (error) {
          return this.invalid;
        }
      };
      host.host_file_read_n = (pathPtr, pathLen, outPtr, inoutLenPtr) => {
        try {
          return readFile(this.text(record, pathPtr, pathLen), outPtr, inoutLenPtr);
        } catch (error) {
          return this.invalid;
        }
      };
      const writeFile = (path, dataPtr, dataLen, append) => {
        if (!this.has(record, this.storage)) return this.denied;
        const fullPath = this.safePath(record.dataPath, path);
        if (!fullPath) return this.invalid;
        try {
          const parent = fullPath.substring(0, fullPath.lastIndexOf('/'));
          this.ensureDir(parent);
          const bytes = this.bytes(record, dataPtr, dataLen);
          if (append) {
            let old = new Uint8Array();
            try {
              old = FS.readFile(fullPath);
            } catch (_) {
            }
            const merged = new Uint8Array(old.length + bytes.length);
            merged.set(old, 0);
            merged.set(bytes, old.length);
            FS.writeFile(fullPath, merged);
          } else {
            FS.writeFile(fullPath, bytes);
          }
          return this.ok;
        } catch (error) {
          return this.setError(error.message);
        }
      };
      host.host_file_write = (pathPtr, dataPtr, dataLen) => {
        try {
          return writeFile(this.cstr(record, pathPtr), dataPtr, dataLen, false);
        } catch (error) {
          return this.invalid;
        }
      };
      host.host_file_append = (pathPtr, dataPtr, dataLen) => {
        try {
          return writeFile(this.cstr(record, pathPtr), dataPtr, dataLen, true);
        } catch (error) {
          return this.invalid;
        }
      };
      host.host_file_write_n = (pathPtr, pathLen, dataPtr, dataLen) => {
        try {
          return writeFile(this.text(record, pathPtr, pathLen), dataPtr, dataLen, false);
        } catch (error) {
          return this.invalid;
        }
      };
      host.host_file_append_n = (pathPtr, pathLen, dataPtr, dataLen) => {
        try {
          return writeFile(this.text(record, pathPtr, pathLen), dataPtr, dataLen, true);
        } catch (error) {
          return this.invalid;
        }
      };
      host.host_config_get = (keyPtr, outPtr, outCap) => {
        if (!this.has(record, this.config)) return this.denied;
        try {
          const path = this.safePath(record.dataPath + '/config', this.cstr(record, keyPtr));
          if (!path) return this.invalid;
          return this.writeText(record, outPtr, outCap, new TextDecoder().decode(FS.readFile(path)));
        } catch (error) {
          return this.notFound;
        }
      };
      host.host_config_set = (keyPtr, valuePtr, valueLen) => {
        if (!this.has(record, this.config)) return this.denied;
        try {
          const path = this.safePath(record.dataPath + '/config', this.cstr(record, keyPtr));
          if (!path) return this.invalid;
          this.ensureDir(path.substring(0, path.lastIndexOf('/')));
          FS.writeFile(path, this.bytes(record, valuePtr, valueLen));
          return this.ok;
        } catch (error) {
          return this.setError(error.message);
        }
      };
      host.host_event_subscribe = (_eventType) => this.has(record, this.events) ? this.ok : this.denied;
      host.host_event_emit = (_eventType, payloadPtr, payloadLen) => {
        if (!this.has(record, this.events)) return this.denied;
        try {
          this.bytes(record, payloadPtr, payloadLen);
          return this.ok;
        } catch (error) {
          return this.invalid;
        }
      };
      return { woki_host: host };
    },
  },

  woki_web_ext_load__deps: ['$WokiExt'],
  woki_web_ext_load: function(idPtr, wasmPathPtr, dataPathPtr, cachePathPtr, permissions) {
    const id = UTF8ToString(idPtr);
    try {
      const record = {
        id,
        dataPath: UTF8ToString(dataPathPtr),
        cachePath: UTF8ToString(cachePathPtr),
        permissions,
        instance: null,
      };
      const bytes = FS.readFile(UTF8ToString(wasmPathPtr));
      const module = new WebAssembly.Module(bytes);
      const instance = new WebAssembly.Instance(module, WokiExt.imports(record));
      record.instance = instance;
      const exports = instance.exports;
      for (const name of ['memory', 'ext_api_version', 'ext_init', 'ext_on_tick', 'ext_on_event', 'ext_on_unload']) {
        if (!exports[name]) {
          return WokiExt.setError(`missing export ${name}`);
        }
      }
      WokiExt.instances.set(id, record);
      WokiExt.lastError = '';
      return 0;
    } catch (error) {
      return WokiExt.setError(error.message);
    }
  },

  woki_web_ext_api_version__deps: ['$WokiExt', '$UTF8ToString'],
  woki_web_ext_api_version: function(idPtr) {
    try {
      return WokiExt.instances.get(UTF8ToString(idPtr)).instance.exports.ext_api_version();
    } catch (error) {
      WokiExt.setError(error.message);
      return -1;
    }
  },

  woki_web_ext_init__deps: ['$WokiExt', '$UTF8ToString'],
  woki_web_ext_init: function(idPtr) {
    try {
      return WokiExt.instances.get(UTF8ToString(idPtr)).instance.exports.ext_init();
    } catch (error) {
      return WokiExt.setError(error.message);
    }
  },

  woki_web_ext_tick__deps: ['$WokiExt', '$UTF8ToString'],
  woki_web_ext_tick: function(idPtr, deltaMs) {
    try {
      WokiExt.instances.get(UTF8ToString(idPtr)).instance.exports.ext_on_tick(deltaMs);
      return 0;
    } catch (error) {
      return WokiExt.setError(error.message);
    }
  },

  woki_web_ext_event__deps: ['$WokiExt', '$UTF8ToString'],
  woki_web_ext_event: function(idPtr, eventType, payloadPtr, payloadLen) {
    const id = UTF8ToString(idPtr);
    const record = WokiExt.instances.get(id);
    try {
      const exports = record.instance.exports;
      let guestPtr = 0;
      if (payloadLen > 0) {
        if (!exports.ext_alloc) {
          return WokiExt.setError('missing ext_alloc for event payload delivery');
        }
        guestPtr = exports.ext_alloc(payloadLen);
        if (guestPtr === 0) {
          return WokiExt.setError('ext_alloc returned null');
        }
        WokiExt.bytes(record, guestPtr, payloadLen).set(HEAPU8.subarray(payloadPtr, payloadPtr + payloadLen));
      }
      exports.ext_on_event(eventType, guestPtr, payloadLen);
      if (guestPtr !== 0 && exports.ext_free) {
        exports.ext_free(guestPtr, payloadLen);
      }
      return 0;
    } catch (error) {
      return WokiExt.setError(error.message);
    }
  },

  woki_web_ext_command__deps: ['$WokiExt', '$UTF8ToString'],
  woki_web_ext_command: function(idPtr, commandIdPtr, payloadPtr, payloadLen) {
    const id = UTF8ToString(idPtr);
    const record = WokiExt.instances.get(id);
    try {
      const exports = record.instance.exports;
      if (!exports.ext_on_command) {
        return WokiExt.setError(`extension ${id} does not export ext_on_command`);
      }
      if (!exports.ext_alloc) {
        return WokiExt.setError(`extension ${id} does not export ext_alloc`);
      }

      const commandBytes = new TextEncoder().encode(UTF8ToString(commandIdPtr));
      const commandGuestPtr = exports.ext_alloc(commandBytes.length);
      if (commandGuestPtr === 0) {
        return WokiExt.setError('ext_alloc returned null for command id');
      }
      WokiExt.bytes(record, commandGuestPtr, commandBytes.length).set(commandBytes);

      let payloadGuestPtr = 0;
      if (payloadLen > 0) {
        payloadGuestPtr = exports.ext_alloc(payloadLen);
        if (payloadGuestPtr === 0) {
          if (exports.ext_free) exports.ext_free(commandGuestPtr, commandBytes.length);
          return WokiExt.setError('ext_alloc returned null for command payload');
        }
        WokiExt.bytes(record, payloadGuestPtr, payloadLen)
          .set(HEAPU8.subarray(payloadPtr, payloadPtr + payloadLen));
      }

      const result = exports.ext_on_command(
        commandGuestPtr, commandBytes.length, payloadGuestPtr, payloadLen);

      if (payloadGuestPtr !== 0 && exports.ext_free) {
        exports.ext_free(payloadGuestPtr, payloadLen);
      }
      if (exports.ext_free) {
        exports.ext_free(commandGuestPtr, commandBytes.length);
      }
      return result;
    } catch (error) {
      return WokiExt.setError(error.message);
    }
  },

  woki_web_ext_unload__deps: ['$WokiExt', '$UTF8ToString'],
  woki_web_ext_unload: function(idPtr) {
    const id = UTF8ToString(idPtr);
    const record = WokiExt.instances.get(id);
    if (!record) return;
    try {
      record.instance.exports.ext_on_unload();
    } catch (_) {
    }
    WokiExt.instances.delete(id);
  },

  woki_web_ext_last_error__deps: ['$WokiExt', '$stringToNewUTF8'],
  woki_web_ext_last_error: function() {
    return stringToNewUTF8(WokiExt.lastError || '');
  },
});
