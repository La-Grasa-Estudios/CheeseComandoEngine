#include <cassert>
#include <fstream>
#include <string>
#include <string_view>
#include <glm/ext.hpp>

#include <AngelScript/AngelScript.h>
#include <angelscript.h>
#include <format>

template<typename T>
void _cdecl GLMVectorConstructor(void* memory)
{
    new(memory) T(0.0f);
}

template<typename T>
void _cdecl GLMVectorConstructor(void* memory, float s)
{
    new(memory) T(s);
}

template<typename T>
void _cdecl GLMVectorConstructor(void* memory, const T& other)
{
    new(memory) T(other);
}

template<typename T>
void _cdecl GLMVectorDestructor(T* self)
{
    self->~T();
}

template<typename VecT, int N>
void _cdecl GLMVectorListConstructor(void* memory, float a, float b = 0.0f, float c = 0.0f, float d = 0.0f)
{
    float values[4] = { a, b, c, d };
    if constexpr (std::is_same_v<VecT, glm::vec2>)
        new(memory) VecT(values[0], values[1]);
    else if constexpr (std::is_same_v<VecT, glm::vec3>)
        new(memory) VecT(values[0], values[1], values[2]);
	else if constexpr (std::is_same_v<VecT, glm::vec4>)
        new(memory) VecT(values[0], values[1], values[2], values[3]);
}

template<typename VecT>
static VecT _cdecl GLMAddWrapper(const VecT& a, const VecT& b) {
    return a + b;
}

template<typename VecT>
static VecT _cdecl GLMSubWrapper(const VecT& a, const VecT& b) {
    return a - b;
}

template<typename VecT>
static VecT _cdecl GLMMulWrapper(const VecT& a, const VecT& b) {
    return a - b;
}

template<typename VecT>
static VecT _cdecl GLMMulScalarWrapper(const VecT& a, const float b) {
    return a * b;
}

template<typename VecT, int N>
static std::string formatGlmVector(const VecT& a)
{
    const char* elements = "xyzw";
    std::string result = "{ ";
    for (int i = 0; i < N; ++i)
    {
        result += std::to_string(a[i]);
        if (i < N - 1)
            result += ", ";
    }
    result += " }";
	return result;
}

template<typename VecT, int N>
void RegisterGLMVector(asIScriptEngine* engine, const std::string& name)
{
    static_assert(N >= 2 && N <= 4);
    const char* elements = "xyzw";

    AS_RETURN_CHECK(engine->RegisterObjectType(name.c_str(), sizeof(VecT), asOBJ_VALUE | asOBJ_APP_CLASS_C));

    AS_RETURN_CHECK(engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_CONSTRUCT, "void f()",
        asFUNCTIONPR(GLMVectorConstructor<VecT>, (void*), void), asCALL_CDECL_OBJFIRST));

    AS_RETURN_CHECK(engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_CONSTRUCT, "void f(float)",
        asFUNCTIONPR(GLMVectorConstructor<VecT>, (void*, float), void), asCALL_CDECL_OBJFIRST));

    AS_RETURN_CHECK(engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_CONSTRUCT, std::format("void f(const {} &in)", name).c_str(),
        asFUNCTIONPR(GLMVectorConstructor<VecT>, (void*, const VecT&), void), asCALL_CDECL_OBJFIRST));

    engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_DESTRUCT, "void f()",
        asFUNCTION(GLMVectorDestructor<VecT>), asCALL_CDECL_OBJLAST);
    
    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{}& opAssign(const {} &in)", name, name).c_str(),
        asMETHODPR(VecT, operator=, (const VecT&), VecT&), asCALL_THISCALL));

    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{} opAdd(const {} &in) const", name, name).c_str(),
        asFUNCTION(GLMAddWrapper<VecT>), asCALL_CDECL_OBJFIRST));

    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{} opSub(const {} &in) const", name, name).c_str(),
        asFUNCTION(GLMSubWrapper<VecT>), asCALL_CDECL_OBJFIRST));

    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{} opMul(const {} &in) const", name, name).c_str(),
        asFUNCTION(GLMMulWrapper<VecT>), asCALL_CDECL_OBJFIRST));

    auto formatter = formatGlmVector<VecT, N>;

    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), "string toString() const",
        asFUNCTION(formatter), asCALL_CDECL_OBJFIRST));

    if constexpr (std::is_same_v<VecT, glm::vec2> || std::is_same_v<VecT, glm::vec3>)
    {
        AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{} opMul(float) const", name).c_str(),
            asFUNCTION(GLMMulScalarWrapper<VecT>), asCALL_CDECL_OBJFIRST));
    }

    std::string decl = "void f(";
    for (int i = 0; i < N; ++i)
        decl += (i > 0 ? ", " : "") + std::string("float");
    decl += ")";

    auto listConst = GLMVectorListConstructor<VecT, N>;

    AS_RETURN_CHECK(engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_CONSTRUCT, decl.c_str(),
        asFUNCTION(listConst), asCALL_CDECL_OBJFIRST));

    for (int i = 0; i < N; ++i)
    {
        std::string prop = std::format("float {}", elements[i]);
        AS_RETURN_CHECK(engine->RegisterObjectProperty(name.c_str(), prop.c_str(), sizeof(float) * i));
    }
}

template<typename T>
static void ConstructMatrixDef(void* memory)
{
    new(memory) T();
}

template<typename T>
static void ConstructMatrix(void* memory, float v)
{
    new(memory) T(v);
}

template<typename T>
static void MatrixDestructorWrapper(T* self)
{
	self->~T();
}

template<typename MatT>
static void RegisterGLMMatrix(asIScriptEngine* engine, const std::string& name)
{
    AS_RETURN_CHECK(engine->RegisterObjectType(name.c_str(), sizeof(MatT), asOBJ_VALUE | asOBJ_APP_CLASS_C));

    AS_RETURN_CHECK(engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_CONSTRUCT, "void f()",
        asFUNCTION(ConstructMatrixDef<MatT>), asCALL_CDECL_OBJLAST));

    AS_RETURN_CHECK(engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_CONSTRUCT, "void f(float)",
		asFUNCTION(ConstructMatrix<MatT>), asCALL_CDECL_OBJFIRST));

	AS_RETURN_CHECK(engine->RegisterObjectBehaviour(name.c_str(), asBEHAVE_DESTRUCT, "void f()",
        asFUNCTION(MatrixDestructorWrapper<MatT>), asCALL_CDECL_OBJLAST));

    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{}& opAssign(const {} &in)", name, name).c_str(),
        asMETHODPR(MatT, operator=, (const MatT&), MatT&), asCALL_THISCALL));

    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{}& opMult(const {} &in)", name, name).c_str(),
        asMETHODPR(MatT, operator*=, (const MatT&), MatT&), asCALL_THISCALL));

    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{}& opMult(float)", name).c_str(),
        asMETHODPR(MatT, operator*=, (const float), MatT&), asCALL_THISCALL));

    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{}& opAdd(const {} &in)", name, name).c_str(),
        asMETHODPR(MatT, operator+=, (const MatT&), MatT&), asCALL_THISCALL));

    AS_RETURN_CHECK(engine->RegisterObjectMethod(name.c_str(), std::format("{}& opSub(const {} &in)", name, name).c_str(),
        asMETHODPR(MatT, operator-=, (const MatT&), MatT&), asCALL_THISCALL));
}

void as_RegisterMath(asIScriptEngine* engine)
{
    RegisterGLMVector<glm::vec2, 2>(engine, "vec2");
    RegisterGLMVector<glm::vec3, 3>(engine, "vec3");
    RegisterGLMVector<glm::vec4, 4>(engine, "vec4");

    RegisterGLMMatrix<glm::mat4>(engine, "mat2");
    RegisterGLMMatrix<glm::mat4>(engine, "mat3");
    RegisterGLMMatrix<glm::mat4>(engine, "mat4");

    AS_RETURN_CHECK(engine->RegisterGlobalFunction("mat4 perspective(float fov, float aspect, float near, float far)",
        asFUNCTION(glm::perspective<float>), asCALL_CDECL));

    auto translateFunc = glm::translate<float, glm::packed_highp>;
    auto scaleFunc = glm::scale<float, glm::packed_highp>;

    AS_RETURN_CHECK(engine->RegisterGlobalFunction("mat4 translate(const mat4 &in mat, const vec3 &in v)",
        asFUNCTION(translateFunc), asCALL_CDECL));

    AS_RETURN_CHECK(engine->RegisterGlobalFunction("mat4 scale(const mat4 &in mat, const vec3 &in v)",
        asFUNCTION(scaleFunc), asCALL_CDECL));

    auto mixFunc = glm::mix<float, float>;

    engine->RegisterGlobalFunction("float mix(float a, float b, float t)",
        asFUNCTION(mixFunc), asCALL_CDECL);

    engine->RegisterGlobalFunction("float min(float a, float b)",
        asFUNCTIONPR(glm::min<float>, (float, float), float), asCALL_CDECL);

    engine->RegisterGlobalFunction("float max(float a, float b)",
        asFUNCTIONPR(glm::max<float>, (float, float), float), asCALL_CDECL);
}