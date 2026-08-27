FROM golang:1.27-alpine3.24 AS build
WORKDIR /src
COPY go.mod ./
COPY . .
RUN CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o /out/woki-api ./cmd/api

FROM alpine:3.24.1
RUN addgroup -S woki && adduser -S -G woki woki
WORKDIR /app
COPY --from=build /out/woki-api /usr/local/bin/woki-api
RUN mkdir -p /app/data && chown -R woki:woki /app
USER woki
EXPOSE 3000
ENTRYPOINT ["woki-api"]
