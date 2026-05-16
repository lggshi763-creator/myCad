#include <mycad/infrastructure/OpenGLRenderAdapter.hpp>

// All OpenGL / Qt headers are intentionally confined to this translation unit.
// No OpenGL type or macro must leak into the public header (ADR-0005).
#include <array>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <QOpenGLFunctions_4_5_Core>

namespace mycad::infrastructure {

// ---------------------------------------------------------------------------
// GLSL shaders — GLSL 450 Core Profile
// ---------------------------------------------------------------------------

namespace {

const char* kVertexShaderSrc = R"glsl(
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 uMVP;
out vec3 vNormal;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vNormal = aNormal;
}
)glsl";

const char* kFragmentShaderSrc = R"glsl(
#version 450 core
in  vec3 vNormal;
out vec4 FragColor;
void main() {
    vec3  L     = normalize(vec3(0.6, 0.8, 0.4));
    float diff  = max(dot(normalize(vNormal), L), 0.0);
    float light = 0.15 + 0.85 * diff;              // ambient + diffuse
    FragColor = vec4(vec3(0.35, 0.65, 1.0) * light, 1.0);
}
)glsl";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// @brief Compiles a GLSL shader stage.
/// @throws std::runtime_error on compilation failure.
GLuint compileShader(QOpenGLFunctions_4_5_Core& gl, GLenum type, const char* src) {
    GLuint sh = gl.glCreateShader(type);
    gl.glShaderSource(sh, 1, &src, nullptr);
    gl.glCompileShader(sh);

    GLint ok = 0;
    gl.glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        gl.glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<std::size_t>(len), '\0');
        gl.glGetShaderInfoLog(sh, len, nullptr, log.data());
        gl.glDeleteShader(sh);
        throw std::runtime_error("Shader compile error: " + log);
    }
    return sh;
}

/// @brief Links a vertex + fragment shader into a program.
/// @throws std::runtime_error on link failure.
GLuint linkProgram(QOpenGLFunctions_4_5_Core& gl, GLuint vert, GLuint frag) {
    GLuint prog = gl.glCreateProgram();
    gl.glAttachShader(prog, vert);
    gl.glAttachShader(prog, frag);
    gl.glLinkProgram(prog);

    GLint ok = 0;
    gl.glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        gl.glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<std::size_t>(len), '\0');
        gl.glGetProgramInfoLog(prog, len, nullptr, log.data());
        gl.glDeleteProgram(prog);
        throw std::runtime_error("Shader link error: " + log);
    }
    return prog;
}

/// @brief Multiplies two column-major 4×4 float matrices: C = A * B.
void mulMat4(const float* __restrict A, const float* __restrict B, float* __restrict C) noexcept {
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.f;
            for (int k = 0; k < 4; ++k)
                sum += A[k * 4 + row] * B[col * 4 + k];
            C[col * 4 + row] = sum;
        }
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// MeshEntry — per-handle GPU resource bundle (DSA-allocated)
// ---------------------------------------------------------------------------

/// @brief VAO + two VBOs (positions, normals) + EBO for one solid.
struct MeshEntry {
    GLuint vao{0};
    GLuint vboPos{0};
    GLuint vboNorm{0};
    GLuint ebo{0};
    GLsizei indexCount{0};
};

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

/// @brief All OpenGL state — confined to this translation unit (PIMPL).
struct OpenGLRenderAdapter::Impl {
    std::unique_ptr<QOpenGLFunctions_4_5_Core> gl;

    GLuint shaderProgram{0};
    GLint locMVP{-1};

    // Per-frame matrices (column-major 4×4 floats).  Initialised to identity.
    std::array<float, 16> viewMat{};
    std::array<float, 16> projMat{};

    // Per-handle GPU resources.
    std::unordered_map<std::uint64_t, MeshEntry> meshes;

    // Diagnostic test triangle — drawn when meshes is empty.
    MeshEntry testTriangle;
    bool hasTestTriangle{false};

    bool initialized{false};
};

// ---------------------------------------------------------------------------
// DSA helpers (static, take gl + entry by reference)
// ---------------------------------------------------------------------------

namespace {

/// @brief Creates and fills a buffer using DSA glNamedBufferStorage (immutable).
GLuint createStaticBuffer(QOpenGLFunctions_4_5_Core& gl, GLsizeiptr size, const void* data) {
    GLuint buf = 0;
    gl.glCreateBuffers(1, &buf);
    gl.glNamedBufferStorage(buf, size, data, 0);
    return buf;
}

/// @brief Configures a VAO attribute via DSA (position or normal, vec3 float).
void bindVec3Attrib(QOpenGLFunctions_4_5_Core& gl,
                    GLuint vao,
                    GLuint attribIndex,
                    GLuint bindingIndex,
                    GLuint vbo) {
    gl.glVertexArrayVertexBuffer(vao, bindingIndex, vbo, 0, 3 * sizeof(float));
    gl.glVertexArrayAttribFormat(vao, attribIndex, 3, GL_FLOAT, GL_FALSE, 0);
    gl.glVertexArrayAttribBinding(vao, attribIndex, bindingIndex);
    gl.glEnableVertexArrayAttrib(vao, attribIndex);
}

/// @brief Frees all GPU resources held by a MeshEntry.
void freeMeshEntry(QOpenGLFunctions_4_5_Core& gl, MeshEntry& e) noexcept {
    if (e.vao != 0u) {
        gl.glDeleteVertexArrays(1, &e.vao);
        e.vao = 0;
    }
    if (e.vboPos != 0u) {
        gl.glDeleteBuffers(1, &e.vboPos);
        e.vboPos = 0;
    }
    if (e.vboNorm != 0u) {
        gl.glDeleteBuffers(1, &e.vboNorm);
        e.vboNorm = 0;
    }
    if (e.ebo != 0u) {
        gl.glDeleteBuffers(1, &e.ebo);
        e.ebo = 0;
    }
    e.indexCount = 0;
}

/// @brief Uploads positions + normals + indices via DSA and fills entry.
MeshEntry uploadEntry(QOpenGLFunctions_4_5_Core& gl,
                      const std::vector<float>& positions,
                      const std::vector<float>& normals,
                      const std::vector<uint32_t>& indices) {
    MeshEntry e;
    e.indexCount = static_cast<GLsizei>(indices.size());

    gl.glCreateVertexArrays(1, &e.vao);

    // Position buffer (binding point 0, attrib location 0)
    e.vboPos = createStaticBuffer(
        gl, static_cast<GLsizeiptr>(positions.size() * sizeof(float)), positions.data());
    bindVec3Attrib(gl, e.vao, 0, 0, e.vboPos);

    // Normal buffer (binding point 1, attrib location 1)
    e.vboNorm = createStaticBuffer(
        gl, static_cast<GLsizeiptr>(normals.size() * sizeof(float)), normals.data());
    bindVec3Attrib(gl, e.vao, 1, 1, e.vboNorm);

    // Element buffer
    e.ebo = createStaticBuffer(
        gl, static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)), indices.data());
    gl.glVertexArrayElementBuffer(e.vao, e.ebo);

    return e;
}

}  // namespace

// ---------------------------------------------------------------------------
// OpenGLRenderAdapter
// ---------------------------------------------------------------------------

OpenGLRenderAdapter::OpenGLRenderAdapter() : impl_{std::make_unique<Impl>()} {
    // Identity matrices.
    auto& v = impl_->viewMat;
    auto& p = impl_->projMat;
    v.fill(0.f);
    p.fill(0.f);
    v[0] = v[5] = v[10] = v[15] = 1.f;
    p[0] = p[5] = p[10] = p[15] = 1.f;
}

OpenGLRenderAdapter::~OpenGLRenderAdapter() {
    if (!impl_->initialized || !impl_->gl) {
        return;
    }
    auto& gl = *impl_->gl;

    for (auto& [id, entry] : impl_->meshes) {
        freeMeshEntry(gl, entry);
    }
    freeMeshEntry(gl, impl_->testTriangle);

    if (impl_->shaderProgram != 0u) {
        gl.glDeleteProgram(impl_->shaderProgram);
    }
}

void OpenGLRenderAdapter::initialize() {
    if (impl_->initialized) {
        return;
    }

    impl_->gl = std::make_unique<QOpenGLFunctions_4_5_Core>();
    impl_->gl->initializeOpenGLFunctions();
    auto& gl = *impl_->gl;

    // Build shader program.
    const GLuint vert = compileShader(gl, GL_VERTEX_SHADER, kVertexShaderSrc);
    const GLuint frag = compileShader(gl, GL_FRAGMENT_SHADER, kFragmentShaderSrc);
    impl_->shaderProgram = linkProgram(gl, vert, frag);
    gl.glDeleteShader(vert);
    gl.glDeleteShader(frag);

    impl_->locMVP = gl.glGetUniformLocation(impl_->shaderProgram, "uMVP");

    gl.glEnable(GL_DEPTH_TEST);
    gl.glEnable(GL_CULL_FACE);

    // --- Diagnostic test triangle ------------------------------------------
    // A flat equilateral triangle centred at the origin (XY plane, normals +Z).
    const std::vector<float> pos = {-1.f, -0.577f, 0.f, 1.f, -0.577f, 0.f, 0.f, 1.155f, 0.f};
    const std::vector<float> norm = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f};
    const std::vector<uint32_t> idx = {0, 1, 2};

    impl_->testTriangle = uploadEntry(gl, pos, norm, idx);
    impl_->hasTestTriangle = true;
    // -------------------------------------------------------------------------

    impl_->initialized = true;
}

// ---------------------------------------------------------------------------
// IRenderPort overrides
// ---------------------------------------------------------------------------

void OpenGLRenderAdapter::uploadMesh(domain::BRepHandle handle, const domain::TriangleMesh& mesh) {
    if (!impl_->initialized) {
        throw std::runtime_error("OpenGLRenderAdapter::uploadMesh called before initialize()");
    }
    auto& gl = *impl_->gl;

    // Replace any existing entry.
    auto it = impl_->meshes.find(handle.id);
    if (it != impl_->meshes.end()) {
        freeMeshEntry(gl, it->second);
        impl_->meshes.erase(it);
    }

    // Build normals (all zero for now — Sprint 0.4 placeholder).
    // TODO(@dev, sprint-0.5): compute per-vertex normals from triangle faces
    const std::vector<float> normals(mesh.vertices.size(), 0.f);

    impl_->meshes[handle.id] = uploadEntry(gl, mesh.vertices, normals, mesh.indices);
}

void OpenGLRenderAdapter::removeMesh(domain::BRepHandle handle) noexcept {
    if (!impl_->initialized || !impl_->gl) {
        return;
    }
    auto it = impl_->meshes.find(handle.id);
    if (it == impl_->meshes.end()) {
        return;
    }
    freeMeshEntry(*impl_->gl, it->second);
    impl_->meshes.erase(it);
}

void OpenGLRenderAdapter::clearAll() noexcept {
    if (!impl_->initialized || !impl_->gl) {
        return;
    }
    auto& gl = *impl_->gl;
    for (auto& [id, entry] : impl_->meshes) {
        freeMeshEntry(gl, entry);
    }
    impl_->meshes.clear();
}

void OpenGLRenderAdapter::setViewMatrix(const float* mat4) noexcept {
    std::memcpy(impl_->viewMat.data(), mat4, 16 * sizeof(float));
}

void OpenGLRenderAdapter::setProjectionMatrix(const float* mat4) noexcept {
    std::memcpy(impl_->projMat.data(), mat4, 16 * sizeof(float));
}

void OpenGLRenderAdapter::render() {
    if (!impl_->initialized) {
        return;
    }
    auto& gl = *impl_->gl;

    gl.glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Compute MVP = proj * view (column-major).
    std::array<float, 16> mvp{};
    mulMat4(impl_->projMat.data(), impl_->viewMat.data(), mvp.data());

    gl.glUseProgram(impl_->shaderProgram);
    gl.glUniformMatrix4fv(impl_->locMVP, 1, GL_FALSE, mvp.data());

    if (impl_->meshes.empty() && impl_->hasTestTriangle) {
        // No real geometry yet — draw the diagnostic test triangle.
        gl.glBindVertexArray(impl_->testTriangle.vao);
        gl.glDrawElements(GL_TRIANGLES, impl_->testTriangle.indexCount, GL_UNSIGNED_INT, nullptr);
    } else {
        for (auto& [id, entry] : impl_->meshes) {
            gl.glBindVertexArray(entry.vao);
            gl.glDrawElements(GL_TRIANGLES, entry.indexCount, GL_UNSIGNED_INT, nullptr);
        }
    }

    gl.glBindVertexArray(0);
    gl.glUseProgram(0);
}

}  // namespace mycad::infrastructure
