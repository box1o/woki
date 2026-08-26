package workspace

import (
	"net/http"
	"strconv"

	domain "github.com/box1o/woki/internal/domain/workspace"
	"github.com/box1o/woki/internal/interfaces/server/middleware"
	service "github.com/box1o/woki/internal/services/workspace"
	"github.com/box1o/woki/pkg/api"
	apperrors "github.com/box1o/woki/pkg/errors"
	"github.com/box1o/woki/pkg/httpx"
	"github.com/box1o/woki/pkg/id"
)

type Handler struct {
	service *service.Service
	auth    *middleware.Authenticator
}

func New(service *service.Service, auth *middleware.Authenticator) *Handler {
	return &Handler{service: service, auth: auth}
}

func (h *Handler) Register(mux *http.ServeMux) {
	mux.Handle("GET /workspaces", h.auth.RequireAny(http.HandlerFunc(h.list)))
	mux.Handle("POST /workspaces", h.auth.RequireAny(http.HandlerFunc(h.create)))
	mux.Handle("DELETE /workspaces/{workspaceID}", h.auth.RequireAny(http.HandlerFunc(h.delete)))
	mux.Handle("GET /workspaces/{workspaceID}/members", h.auth.RequireAny(http.HandlerFunc(h.members)))
	mux.Handle("GET /workspaces/{workspaceID}/member-candidates", h.auth.RequireAny(http.HandlerFunc(h.memberCandidates)))
	mux.Handle("POST /workspaces/{workspaceID}/members", h.auth.RequireAny(http.HandlerFunc(h.addMember)))
	mux.Handle("DELETE /workspaces/{workspaceID}/members/{memberID}", h.auth.RequireAny(http.HandlerFunc(h.removeMember)))
	mux.Handle("PATCH /workspaces/{workspaceID}/members/{memberID}", h.auth.RequireAny(http.HandlerFunc(h.updateRole)))
}

func (h *Handler) memberCandidates(w http.ResponseWriter, r *http.Request) {
	actor, ok := h.actor(w, r)
	if !ok {
		return
	}
	workspaceID, err := id.Parse(r.PathValue("workspaceID"))
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	limit := 8
	if raw := r.URL.Query().Get("limit"); raw != "" {
		parsed, err := strconv.Atoi(raw)
		if err != nil || parsed < 1 || parsed > 20 {
			httpx.WriteMappedError(w, service.ErrSearchQuery.WithDetail("limit must be an integer between 1 and 20"))
			return
		}
		limit = parsed
	}
	values, err := h.service.SearchMemberCandidates(
		r.Context(),
		actor,
		workspaceID,
		r.URL.Query().Get("q"),
		limit,
	)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	out := make([]api.User, 0, len(values))
	for _, value := range values {
		out = append(out, api.User{
			ID:        value.ID.String(),
			Email:     value.Email,
			Name:      value.Name,
			AvatarURL: value.AvatarURL,
		})
	}
	httpx.WriteJSON(w, http.StatusOK, out)
}

func (h *Handler) actor(w http.ResponseWriter, r *http.Request) (id.ID, bool) {
	principal, ok := middleware.PrincipalFrom(r.Context())
	if !ok || principal.User == nil {
		apperrors.WriteError(w, apperrors.ErrUnauthorized)
		return "", false
	}
	return principal.User.ID, true
}

func (h *Handler) create(w http.ResponseWriter, r *http.Request) {
	actor, ok := h.actor(w, r)
	if !ok {
		return
	}
	var req struct {
		Name string `json:"name"`
	}
	if err := httpx.DecodeJSON(w, r, &req); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	value, err := h.service.Create(r.Context(), actor, req.Name)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	httpx.WriteJSON(w, http.StatusCreated, toAPIWorkspace(value))
}

func (h *Handler) list(w http.ResponseWriter, r *http.Request) {
	actor, ok := h.actor(w, r)
	if !ok {
		return
	}
	values, err := h.service.List(r.Context(), actor)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	out := make([]api.Workspace, 0, len(values))
	for _, value := range values {
		out = append(out, toAPIWorkspace(value))
	}
	httpx.WriteJSON(w, http.StatusOK, out)
}

func (h *Handler) delete(w http.ResponseWriter, r *http.Request) {
	actor, ok := h.actor(w, r)
	if !ok {
		return
	}
	workspaceID, err := id.Parse(r.PathValue("workspaceID"))
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	if err := h.service.Delete(r.Context(), actor, workspaceID); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	w.WriteHeader(http.StatusNoContent)
}

func (h *Handler) members(w http.ResponseWriter, r *http.Request) {
	actor, ok := h.actor(w, r)
	if !ok {
		return
	}
	workspaceID, err := id.Parse(r.PathValue("workspaceID"))
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	values, err := h.service.ListMembers(r.Context(), actor, workspaceID)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	out := make([]api.Member, 0, len(values))
	for _, value := range values {
		out = append(out, toAPIMember(value))
	}
	httpx.WriteJSON(w, http.StatusOK, out)
}

func (h *Handler) addMember(w http.ResponseWriter, r *http.Request) {
	actor, ok := h.actor(w, r)
	if !ok {
		return
	}
	workspaceID, err := id.Parse(r.PathValue("workspaceID"))
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	var req struct {
		Email string `json:"email"`
		Role  string `json:"role"`
	}
	if err := httpx.DecodeJSON(w, r, &req); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	role, err := domain.ParseRole(req.Role)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	value, err := h.service.AddMember(r.Context(), actor, workspaceID, req.Email, role)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	httpx.WriteJSON(w, http.StatusCreated, toAPIMember(value))
}

func (h *Handler) removeMember(w http.ResponseWriter, r *http.Request) {
	actor, workspaceID, memberID, ok := h.ids(w, r)
	if !ok {
		return
	}
	if err := h.service.RemoveMember(r.Context(), actor, workspaceID, memberID); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	w.WriteHeader(http.StatusNoContent)
}

func (h *Handler) updateRole(w http.ResponseWriter, r *http.Request) {
	actor, workspaceID, memberID, ok := h.ids(w, r)
	if !ok {
		return
	}
	var req struct {
		Role string `json:"role"`
	}
	if err := httpx.DecodeJSON(w, r, &req); err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	role, err := domain.ParseRole(req.Role)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	value, err := h.service.UpdateMemberRole(r.Context(), actor, workspaceID, memberID, role)
	if err != nil {
		httpx.WriteMappedError(w, err)
		return
	}
	httpx.WriteJSON(w, http.StatusOK, toAPIMember(value))
}

func (h *Handler) ids(w http.ResponseWriter, r *http.Request) (id.ID, id.ID, id.ID, bool) {
	actor, ok := h.actor(w, r)
	if !ok {
		return "", "", "", false
	}
	workspaceID, err := id.Parse(r.PathValue("workspaceID"))
	if err != nil {
		httpx.WriteMappedError(w, err)
		return actor, "", "", false
	}
	memberID, err := id.Parse(r.PathValue("memberID"))
	if err != nil {
		httpx.WriteMappedError(w, err)
		return actor, workspaceID, "", false
	}
	return actor, workspaceID, memberID, true
}

func toAPIWorkspace(value *domain.Workspace) api.Workspace {
	return api.Workspace{
		ID:        value.ID.String(),
		Name:      value.Name,
		OwnerID:   value.OwnerID.String(),
		CreatedAt: value.CreatedAt,
		UpdatedAt: value.UpdatedAt,
	}
}

func toAPIMember(value *domain.Member) api.Member {
	return api.Member{
		ID:          value.ID.String(),
		UserID:      value.UserID.String(),
		WorkspaceID: value.WorkspaceID.String(),
		Email:       value.Email,
		Name:        value.Name,
		AvatarURL:   value.AvatarURL,
		Role:        string(value.Role),
		CreatedAt:   value.CreatedAt,
		UpdatedAt:   value.UpdatedAt,
	}
}
