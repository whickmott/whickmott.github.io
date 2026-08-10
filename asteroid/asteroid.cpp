#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if !defined(ASTEROID_WASM)
#include <iomanip>
#include <iostream>
#endif

#if defined(ASTEROID_WASM)
#define AST_EXPORT __attribute__((visibility("default")))
#else
#define AST_EXPORT
#endif

constexpr float PI = 3.14159265358979323846f;

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Tri {
    int a = 0;
    int b = 0;
    int c = 0;
};

struct LobeShape {
    float axisX = 1.0f;
    float axisY = 0.9f;
    float axisZ = 0.8f;
    float ridge = 0.0f;
    float taper = 0.0f;
    float asymmetry = 0.0f;
    float macro = 0.12f;
    float roughness = 0.04f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
};

struct CraterSettings {
    int count = 8;
    float minRadius = 0.04f;
    float maxRadius = 0.14f;
    float depth = 0.45f;
    float rim = 0.25f;
    float exponent = 1.8f;
    float freshness = 0.7f;
};

struct BoulderSettings {
    int count = 0;
    float minRadius = 0.01f;
    float maxRadius = 0.05f;
    float height = 0.45f;
    float sharpness = 0.55f;
};

struct TerrainSettings {
    int facetCount = 0;
    float facetStrength = 0.0f;
    int grooveCount = 0;
    float grooveWidth = 0.03f;
    float grooveDepth = 0.12f;
    float grooveLength = 0.25f;
};

struct NeckSettings {
    float length = 0.10f;
    float radiusY = 0.28f;
    float radiusZ = 0.28f;
    float flare = 0.70f;
    float profile = 1.60f;
    float inset = 0.18f;
    float smoothing = 0.035f;
    float surfaceInheritance = 1.00f;
    float featureCarryover = 0.75f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
};

struct Crater {
    Vec3 direction;
    float depth = 1.0f;
    float rimHeight = 0.0f;
    float invChordRadius = 1.0f;
    float supportChordSq = 0.0f;
    float supportAngle = 0.1f;
    float softness = 1.0f;
};

struct Boulder {
    Vec3 direction;
    float height = 1.0f;
    float invChordRadius = 1.0f;
    float supportChordSq = 0.0f;
    float supportAngle = 0.1f;
    float sharpness = 1.0f;
};

struct Facet {
    Vec3 direction;
    float supportCos = 0.9f;
    float supportAngle = 0.3f;
    float strength = 0.0f;
};

struct Groove {
    Vec3 centre;
    Vec3 tangent;
    Vec3 bitangent;
    float width = 0.05f;
    float halfLength = 0.2f;
    float depth = 1.0f;
    float supportCos = 0.7f;
    float supportAngle = 0.4f;
};

struct NeckCrater {
    float x = 0.0f;
    float angle = 0.0f;
    float radius = 1.0f;
    float depth = 1.0f;
    float rimHeight = 0.0f;
    float softness = 1.0f;
};

struct NeckBoulder {
    float x = 0.0f;
    float angle = 0.0f;
    float radius = 1.0f;
    float height = 1.0f;
    float sharpness = 1.0f;
};

struct Config {
    float size = 100.0f;
    std::string topology = "single";
    std::array<LobeShape, 2> shape{};
    float lobeRatio = 0.72f;
    float separation = 1.20f;
    NeckSettings neck{};
    std::array<CraterSettings, 2> crater{};
    std::array<BoulderSettings, 2> boulder{};
    std::array<TerrainSettings, 2> terrain{};
    int resolution = 72;
    float adaptiveDetail = 0.0f;
    int seed = 42;
};

struct LobeLayout {
    Vec3 c1;
    Vec3 c2;
    float r1 = 1.0f;
    float r2 = 1.0f;
};

struct LobeBounds {
    float x = 1.0f;
    float y = 1.0f;
    float z = 1.0f;
};

struct LobeBasis {
    Vec3 x{1.0f, 0.0f, 0.0f};
    Vec3 y{0.0f, 1.0f, 0.0f};
    Vec3 z{0.0f, 0.0f, 1.0f};
};

struct MeshDiagnostics {
    size_t boundaryEdges = 0;
    size_t nonManifoldEdges = 0;
    size_t windingErrors = 0;
    size_t components = 0;
};

struct SurfaceColourData {
    int lobe = 0;
    float lobeBlend = 0.0f;
    float macro = 0.0f;
    float roughness = 0.0f;
    float craterBowl = 0.0f;
    float craterRim = 0.0f;
    float boulder = 0.0f;
    float facet = 0.0f;
    float groove = 0.0f;
    float totalShift = 0.0f;
};

static Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3 operator*(const Vec3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static Vec3 operator/(const Vec3& a, float s) { return {a.x / s, a.y / s, a.z / s}; }

static float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
static float length(const Vec3& v) { return std::sqrt(dot(v, v)); }
static Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }
static Vec3 normalise(const Vec3& v) {
    const float len = length(v);
    return len > 1.0e-8f ? v / len : Vec3{0.0f, 1.0f, 0.0f};
}

static float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
static float mixf(float a, float b, float t) { return a + (b - a) * t; }
static float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }
static float smootherstep(float t) {
    t = clampf(t, 0.0f, 1.0f);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static uint32_t hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

static std::string generatedAsteroidName(int seed) {
    static const std::array<const char*, 27> first = {
        "Ara", "Bel", "Cer", "Dra", "Eri", "Fae", "Gal", "Hel", "Ira",
        "Kae", "Kor", "Lun", "Myr", "Ner", "Oph", "Or", "Pel", "Qua",
        "Rhe", "Sol", "Tal", "Umb", "Val", "Vel", "Xan", "Yar", "Zen"
    };
    static const std::array<const char*, 27> ending = {
        "dor", "lia", "nax", "ris", "tus", "ven", "ria", "mos", "len",
        "xis", "ron", "thys", "via", "dus", "nor", "sar", "thea", "mir",
        "phos", "rion", "vara", "neth", "lios", "cara", "sune", "dax", "riel"
    };

    uint32_t state = hash32(static_cast<uint32_t>(seed) ^ 0x4e414d45u);
    std::string name = first[state % first.size()];
    state = hash32(state ^ 0x9e3779b9u);
    std::string tail = ending[state % ending.size()];
    if (!name.empty() && !tail.empty() && name.back() == tail.front()) tail.erase(tail.begin());
    name += tail;
    return name;
}

static std::string generatedObjectId(int seed) {
    static constexpr char alphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    constexpr uint32_t mask = 31u;

    uint32_t value = hash32(static_cast<uint32_t>(seed) ^ 0xa57e10f1u);
    char encoded[8]{};
    for (int i = 6; i >= 0; --i) {
        encoded[i] = alphabet[value & mask];
        value >>= 5;
    }

    std::string code = "AF-";
    code.append(encoded, encoded + 4);
    code += '-';
    code.append(encoded + 4, encoded + 7);
    return code;
}

static uint32_t hash4(int x, int y, int z, int seed) {
    uint32_t h = static_cast<uint32_t>(seed) ^ 0x9e3779b9u;
    h = hash32(h ^ static_cast<uint32_t>(x) * 0x85ebca6bu);
    h = hash32(h ^ static_cast<uint32_t>(y) * 0xc2b2ae35u);
    h = hash32(h ^ static_cast<uint32_t>(z) * 0x27d4eb2fu);
    return h;
}

static float hashUnit(int x, int y, int z, int seed) {
    return (static_cast<float>(hash4(x, y, z, seed)) / 4294967295.0f) * 2.0f - 1.0f;
}

static float valueNoise(const Vec3& p, int seed) {
    const int x0 = static_cast<int>(std::floor(p.x));
    const int y0 = static_cast<int>(std::floor(p.y));
    const int z0 = static_cast<int>(std::floor(p.z));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;
    const int z1 = z0 + 1;
    const float tx = smoothstep(p.x - static_cast<float>(x0));
    const float ty = smoothstep(p.y - static_cast<float>(y0));
    const float tz = smoothstep(p.z - static_cast<float>(z0));

    const float c000 = hashUnit(x0, y0, z0, seed);
    const float c100 = hashUnit(x1, y0, z0, seed);
    const float c010 = hashUnit(x0, y1, z0, seed);
    const float c110 = hashUnit(x1, y1, z0, seed);
    const float c001 = hashUnit(x0, y0, z1, seed);
    const float c101 = hashUnit(x1, y0, z1, seed);
    const float c011 = hashUnit(x0, y1, z1, seed);
    const float c111 = hashUnit(x1, y1, z1, seed);

    const float x00 = mixf(c000, c100, tx);
    const float x10 = mixf(c010, c110, tx);
    const float x01 = mixf(c001, c101, tx);
    const float x11 = mixf(c011, c111, tx);
    return mixf(mixf(x00, x10, ty), mixf(x01, x11, ty), tz);
}

static float fbm(Vec3 p, int seed, int octaves) {
    float amplitude = 0.5f;
    float total = 0.0f;
    float norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        total += amplitude * valueNoise(p, seed + i * 1013);
        norm += amplitude;
        p = p * 2.03f + Vec3{17.7f, -11.3f, 9.1f};
        amplitude *= 0.5f;
    }
    return norm > 0.0f ? total / norm : 0.0f;
}

static float rand01(uint32_t& state) {
    state = hash32(state + 0x9e3779b9u);
    return static_cast<float>(state) / 4294967295.0f;
}

static Vec3 randomUnitVector(uint32_t& state) {
    const float z = rand01(state) * 2.0f - 1.0f;
    const float a = rand01(state) * 2.0f * PI;
    const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    return {r * std::cos(a), z, r * std::sin(a)};
}

static std::string urlDecode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') out.push_back(' ');
        else if (s[i] == '%' && i + 2 < s.size()) {
            const std::string hex = s.substr(i + 1, 2);
            char* end = nullptr;
            const long v = std::strtol(hex.c_str(), &end, 16);
            if (end && *end == '\0') {
                out.push_back(static_cast<char>(v));
                i += 2;
            } else out.push_back(s[i]);
        } else out.push_back(s[i]);
    }
    return out;
}

static std::unordered_map<std::string, std::string> parseQuery(const char* query) {
    std::unordered_map<std::string, std::string> result;
    if (!query) return result;
    const std::string source(query);
    size_t start = 0;
    while (start <= source.size()) {
        const size_t amp = source.find('&', start);
        const size_t end = amp == std::string::npos ? source.size() : amp;
        const size_t eq = source.find('=', start);
        if (eq != std::string::npos && eq < end) {
            result[urlDecode(source.substr(start, eq - start))] = urlDecode(source.substr(eq + 1, end - eq - 1));
        }
        if (amp == std::string::npos) break;
        start = amp + 1;
    }
    return result;
}

static float getFloat(const std::unordered_map<std::string, std::string>& q, const std::string& key, float fallback) {
    const auto it = q.find(key);
    if (it == q.end() || it->second.empty()) return fallback;
    char* end = nullptr;
    const float value = std::strtof(it->second.c_str(), &end);
    return end && *end == '\0' ? value : fallback;
}

static int getInt(const std::unordered_map<std::string, std::string>& q, const std::string& key, int fallback) {
    const auto it = q.find(key);
    if (it == q.end() || it->second.empty()) return fallback;
    char* end = nullptr;
    const long value = std::strtol(it->second.c_str(), &end, 10);
    if (!end || *end != '\0') return fallback;
    if (value < -2147483647L - 1L || value > 2147483647L) return fallback;
    return static_cast<int>(value);
}

static std::string getString(const std::unordered_map<std::string, std::string>& q, const std::string& key, const std::string& fallback) {
    const auto it = q.find(key);
    return it == q.end() ? fallback : it->second;
}

static Config loadConfigFromQuery(const char* query) {
    Config c;
    c.shape[1] = c.shape[0];
    c.crater[1] = c.crater[0];
    c.boulder[1] = c.boulder[0];
    c.terrain[1] = c.terrain[0];

    const auto q = parseQuery(query);
    c.size = clampf(getFloat(q, "size", c.size), 5.0f, 500.0f);
    c.topology = getString(q, "topology", c.topology);
    if (c.topology != "single" && c.topology != "contact" && c.topology != "detached") c.topology = "single";
    c.lobeRatio = clampf(getFloat(q, "lobeRatio", c.lobeRatio), 0.20f, 1.50f);
    c.separation = clampf(getFloat(q, "separation", c.separation), 1.02f, 3.0f);
    c.neck.length = clampf(getFloat(q, "neckLength", c.neck.length), 0.0f, 0.80f);
    c.neck.radiusY = clampf(getFloat(q, "neckRadiusY", c.neck.radiusY), 0.04f, 0.85f);
    c.neck.radiusZ = clampf(getFloat(q, "neckRadiusZ", c.neck.radiusZ), 0.04f, 0.85f);
    c.neck.flare = clampf(getFloat(q, "neckFlare", c.neck.flare), 0.0f, 2.50f);
    c.neck.profile = clampf(getFloat(q, "neckProfile", c.neck.profile), 0.30f, 4.00f);
    c.neck.inset = clampf(getFloat(q, "neckInset", c.neck.inset), 0.02f, 0.60f);
    c.neck.smoothing = clampf(getFloat(q, "neckSmoothing", c.neck.smoothing), 0.0f, 0.20f);
    c.neck.surfaceInheritance = clampf(getFloat(q, "neckSurfaceInheritance", c.neck.surfaceInheritance), 0.0f, 1.50f);
    c.neck.featureCarryover = clampf(getFloat(q, "neckFeatureCarryover", c.neck.featureCarryover), 0.0f, 1.50f);
    c.neck.offsetY = clampf(getFloat(q, "neckOffsetY", c.neck.offsetY), -0.50f, 0.50f);
    c.neck.offsetZ = clampf(getFloat(q, "neckOffsetZ", c.neck.offsetZ), -0.50f, 0.50f);

    for (int lobe = 0; lobe < 2; ++lobe) {
        const std::string suffix = lobe == 0 ? "" : "2";
        LobeShape& s = c.shape[lobe];
        const LobeShape fallback = lobe == 0 ? c.shape[0] : c.shape[0];
        s.axisX = clampf(getFloat(q, "axisX" + suffix, fallback.axisX), 0.35f, 3.5f);
        s.axisY = clampf(getFloat(q, "axisY" + suffix, fallback.axisY), 0.35f, 3.5f);
        s.axisZ = clampf(getFloat(q, "axisZ" + suffix, fallback.axisZ), 0.35f, 3.5f);
        s.ridge = clampf(getFloat(q, "ridge" + suffix, fallback.ridge), 0.0f, 0.80f);
        s.taper = clampf(getFloat(q, "taper" + suffix, fallback.taper), -0.85f, 0.85f);
        s.asymmetry = clampf(getFloat(q, "asymmetry" + suffix, fallback.asymmetry), -0.70f, 0.70f);
        s.macro = clampf(getFloat(q, "macro" + suffix, fallback.macro), 0.0f, 0.50f);
        s.roughness = clampf(getFloat(q, "roughness" + suffix, fallback.roughness), 0.0f, 0.22f);
        s.rotationX = clampf(getFloat(q, "rotationX" + suffix, fallback.rotationX), -180.0f, 180.0f);
        s.rotationY = clampf(getFloat(q, "rotationY" + suffix, fallback.rotationY), -180.0f, 180.0f);
        s.rotationZ = clampf(getFloat(q, "rotationZ" + suffix, fallback.rotationZ), -180.0f, 180.0f);

        CraterSettings& cr = c.crater[lobe];
        const CraterSettings crFallback = lobe == 0 ? c.crater[0] : c.crater[0];
        cr.count = std::clamp(getInt(q, "craterCount" + suffix, crFallback.count), 0, 1200);
        cr.minRadius = clampf(getFloat(q, "craterMin" + suffix, crFallback.minRadius), 0.003f, 0.25f);
        cr.maxRadius = clampf(getFloat(q, "craterMax" + suffix, crFallback.maxRadius), cr.minRadius, 0.45f);
        cr.depth = clampf(getFloat(q, "craterDepth" + suffix, crFallback.depth), 0.05f, 1.2f);
        cr.rim = clampf(getFloat(q, (lobe == 0 ? "rim" : "rim2"), crFallback.rim), 0.0f, 0.8f);
        cr.exponent = clampf(getFloat(q, "craterExponent" + suffix, crFallback.exponent), 0.2f, 5.0f);
        cr.freshness = clampf(getFloat(q, "craterFreshness" + suffix, crFallback.freshness), 0.0f, 1.0f);

        BoulderSettings& b = c.boulder[lobe];
        const BoulderSettings bFallback = lobe == 0 ? c.boulder[0] : c.boulder[0];
        b.count = std::clamp(getInt(q, "boulderCount" + suffix, bFallback.count), 0, 600);
        b.minRadius = clampf(getFloat(q, "boulderMin" + suffix, bFallback.minRadius), 0.002f, 0.15f);
        b.maxRadius = clampf(getFloat(q, "boulderMax" + suffix, bFallback.maxRadius), b.minRadius, 0.22f);
        b.height = clampf(getFloat(q, "boulderHeight" + suffix, bFallback.height), 0.05f, 1.5f);
        b.sharpness = clampf(getFloat(q, "boulderSharpness" + suffix, bFallback.sharpness), 0.0f, 1.0f);

        TerrainSettings& t = c.terrain[lobe];
        const TerrainSettings tFallback = lobe == 0 ? c.terrain[0] : c.terrain[0];
        t.facetCount = std::clamp(getInt(q, "facetCount" + suffix, tFallback.facetCount), 0, 32);
        t.facetStrength = clampf(getFloat(q, "facetStrength" + suffix, tFallback.facetStrength), 0.0f, 1.0f);
        t.grooveCount = std::clamp(getInt(q, "grooveCount" + suffix, tFallback.grooveCount), 0, 96);
        t.grooveWidth = clampf(getFloat(q, "grooveWidth" + suffix, tFallback.grooveWidth), 0.005f, 0.22f);
        t.grooveDepth = clampf(getFloat(q, "grooveDepth" + suffix, tFallback.grooveDepth), 0.005f, 0.60f);
        t.grooveLength = clampf(getFloat(q, "grooveLength" + suffix, tFallback.grooveLength), 0.03f, 1.2f);
    }

    c.resolution = std::clamp(getInt(q, "resolution", c.resolution), 28, 320);
    c.adaptiveDetail = clampf(getFloat(q, "adaptiveDetail", c.adaptiveDetail), 0.0f, 1.0f);
    c.seed = getInt(q, "seed", c.seed);
    return c;
}

static Config loadConfig() {
    return loadConfigFromQuery(std::getenv("QUERY_STRING"));
}

class DirectionIndex {
public:
    static constexpr int N = 12;
    static constexpr int BIN_COUNT = 6 * N * N;

    DirectionIndex() : bins_(BIN_COUNT) {}

    void clear() {
        for (auto& bin : bins_) bin.clear();
    }

    void add(int featureIndex, const Vec3& direction, float supportAngle) {
        const float expanded = std::min(PI, supportAngle + 0.17f);
        const float threshold = std::cos(expanded);
        for (int bin = 0; bin < BIN_COUNT; ++bin) {
            if (dot(direction, binCentre(bin)) >= threshold) bins_[bin].push_back(featureIndex);
        }
    }

    const std::vector<int>& query(const Vec3& direction) const {
        return bins_[binIndex(direction)];
    }

private:
    std::vector<std::vector<int>> bins_;

    static int clampCell(int v) { return std::max(0, std::min(N - 1, v)); }

    static int binIndex(const Vec3& d) {
        const float ax = std::fabs(d.x);
        const float ay = std::fabs(d.y);
        const float az = std::fabs(d.z);
        int face = 0;
        float u = 0.0f;
        float v = 0.0f;

        if (ax >= ay && ax >= az) {
            if (d.x >= 0.0f) { face = 0; u = -d.z / ax; v = d.y / ax; }
            else { face = 1; u = d.z / ax; v = d.y / ax; }
        } else if (ay >= ax && ay >= az) {
            if (d.y >= 0.0f) { face = 2; u = d.x / ay; v = -d.z / ay; }
            else { face = 3; u = d.x / ay; v = d.z / ay; }
        } else {
            if (d.z >= 0.0f) { face = 4; u = d.x / az; v = d.y / az; }
            else { face = 5; u = -d.x / az; v = d.y / az; }
        }

        const int ix = clampCell(static_cast<int>(std::floor((u + 1.0f) * 0.5f * N)));
        const int iy = clampCell(static_cast<int>(std::floor((v + 1.0f) * 0.5f * N)));
        return face * N * N + iy * N + ix;
    }

    static Vec3 binCentre(int index) {
        const int face = index / (N * N);
        const int rem = index % (N * N);
        const int iy = rem / N;
        const int ix = rem % N;
        const float u = -1.0f + (static_cast<float>(ix) + 0.5f) * (2.0f / N);
        const float v = -1.0f + (static_cast<float>(iy) + 0.5f) * (2.0f / N);
        Vec3 p;
        switch (face) {
            case 0: p = {1.0f, v, -u}; break;
            case 1: p = {-1.0f, v, u}; break;
            case 2: p = {u, 1.0f, -v}; break;
            case 3: p = {u, -1.0f, v}; break;
            case 4: p = {u, v, 1.0f}; break;
            default: p = {-u, v, -1.0f}; break;
        }
        return normalise(p);
    }
};

class AsteroidField {
public:
    explicit AsteroidField(Config config) : c_(std::move(config)) {
        layout_.r1 = c_.size;
        layout_.r2 = c_.size * c_.lobeRatio;
        buildBases();
        buildFeatures(0);
        if (c_.topology != "single") buildFeatures(1);
        buildIndices();
        updateLobeBounds();
        finaliseLayout();
    }

    float sample(const Vec3& p) const {
        const float d1 = lobeSurfaceSdf(p, layout_.c1, layout_.r1, 0);
        if (c_.topology == "single") return d1;

        const float d2 = lobeSurfaceSdf(p, layout_.c2, layout_.r2, 1);
        if (c_.topology == "detached") return std::min(d1, d2);

        const float bridgeField = contactBridgeField(p, d1, d2);

        // Never let the bridge ownership regions cut either physical lobe away.
        // The bridge remains the connector in the gap, while both complete lobe
        // fields are retained everywhere they actually exist.
        return std::min(bridgeField, std::min(d1, d2));
    }

    Vec3 centre(int lobe) const {
        return lobe == 0 ? layout_.c1 : layout_.c2;
    }

    float lobeRadius(int lobe) const {
        return lobe == 0 ? layout_.r1 : layout_.r2;
    }

    LobeBasis lobeBasis(int lobe) const {
        return basis_[lobe];
    }

    SurfaceColourData surfaceColourData(const Vec3& p) const {
        if (c_.topology == "contact") {
            const SurfaceColourData left = directionalSurfaceData(surfaceDirectionForWorldPoint(p, 0), 0);
            const SurfaceColourData right = directionalSurfaceData(surfaceDirectionForWorldPoint(p, 1), 1);

            // If a rotated/elongated lobe crosses a bridge ownership plane, the
            // full lobe is preserved by sample(). Keep its material/feature data
            // with that lobe as well instead of recolouring it as neck/other body.
            const float d1 = lobeSurfaceSdf(p, layout_.c1, layout_.r1, 0);
            const float d2 = lobeSurfaceSdf(p, layout_.c2, layout_.r2, 1);
            const float db = contactBridgeField(p, d1, d2);
            const float a1 = std::fabs(d1);
            const float a2 = std::fabs(d2);
            const float ab = std::fabs(db);
            const float ownershipEpsilon = std::max(c_.size * 1.0e-5f, 1.0e-5f);
            if (a1 + ownershipEpsilon < ab && a1 <= a2) return left;
            if (a2 + ownershipEpsilon < ab && a2 < a1) return right;

            const float span = std::max(neckEndX_ - neckStartX_, 1.0e-5f);
            const float t = clampf((p.x - neckStartX_) / span, 0.0f, 1.0f);
            const SurfaceColourData neck = neckSurfaceData(p, t, true);

            const float leftLo = neckStartX_ - neckLeftTransition_;
            const float leftHi = neckStartX_ + neckLeftTransition_;
            const float rightLo = neckEndX_ - neckRightTransition_;
            const float rightHi = neckEndX_ + neckRightTransition_;

            if (p.x <= leftLo) return left;
            if (p.x < leftHi) {
                const float u = smootherstep((p.x - leftLo) / std::max(leftHi - leftLo, 1.0e-6f));
                return blendSurfaceData(left, neck, u);
            }
            if (p.x <= rightLo) return neck;
            if (p.x < rightHi) {
                const float u = smootherstep((p.x - rightLo) / std::max(rightHi - rightLo, 1.0e-6f));
                return blendSurfaceData(neck, right, u);
            }
            return right;
        }

        int lobe = 0;
        if (c_.topology != "single") {
            const float d0 = std::fabs(lobeSurfaceSdf(p, layout_.c1, layout_.r1, 0));
            const float d1 = std::fabs(lobeSurfaceSdf(p, layout_.c2, layout_.r2, 1));
            lobe = d1 < d0 ? 1 : 0;
        }
        return directionalSurfaceData(surfaceDirectionForWorldPoint(p, lobe), lobe);
    }

    void bounds(Vec3& mn, Vec3& mx) const {
        mn = {layout_.c1.x - bounds_[0].x, layout_.c1.y - bounds_[0].y, layout_.c1.z - bounds_[0].z};
        mx = {layout_.c1.x + bounds_[0].x, layout_.c1.y + bounds_[0].y, layout_.c1.z + bounds_[0].z};
        if (c_.topology != "single") {
            mn.x = std::min(mn.x, layout_.c2.x - bounds_[1].x);
            mn.y = std::min(mn.y, layout_.c2.y - bounds_[1].y);
            mn.z = std::min(mn.z, layout_.c2.z - bounds_[1].z);
            mx.x = std::max(mx.x, layout_.c2.x + bounds_[1].x);
            mx.y = std::max(mx.y, layout_.c2.y + bounds_[1].y);
            mx.z = std::max(mx.z, layout_.c2.z + bounds_[1].z);
        }
        if (c_.topology == "contact") {
            mn.x = std::min(mn.x, neckStartX_);
            mx.x = std::max(mx.x, neckEndX_);
        }
        const float pad = c_.size * 0.08f;
        mn = mn - Vec3{pad, pad, pad};
        mx = mx + Vec3{pad, pad, pad};
    }

private:
    Config c_;
    LobeLayout layout_;
    std::array<LobeBounds, 2> bounds_{};
    std::array<LobeBasis, 2> basis_{};
    std::array<std::vector<Crater>, 2> craters_;
    std::array<std::vector<Boulder>, 2> boulders_;
    std::array<std::vector<Facet>, 2> facets_;
    std::array<std::vector<Groove>, 2> grooves_;
    std::vector<NeckCrater> neckCraters_;
    std::vector<NeckBoulder> neckBoulders_;
    std::array<DirectionIndex, 2> craterIndex_;
    std::array<DirectionIndex, 2> boulderIndex_;
    std::array<DirectionIndex, 2> facetIndex_;
    std::array<DirectionIndex, 2> grooveIndex_;
    static constexpr int NECK_CONTOUR_SAMPLES = 128;
    float neckStartX_ = 0.0f;
    float neckEndX_ = 0.0f;
    float neckCentreY_ = 0.0f;
    float neckCentreZ_ = 0.0f;
    float neckThroatY_ = 1.0f;
    float neckThroatZ_ = 1.0f;
    float neckLeftTransition_ = 1.0f;
    float neckRightTransition_ = 1.0f;
    std::array<float, NECK_CONTOUR_SAMPLES> neckLeftRaw_{};
    std::array<float, NECK_CONTOUR_SAMPLES> neckRightRaw_{};
    std::array<float, NECK_CONTOUR_SAMPLES> neckLeftSmooth_{};
    std::array<float, NECK_CONTOUR_SAMPLES> neckRightSmooth_{};

    float radiusForLobe(int lobe) const { return lobe == 0 ? layout_.r1 : layout_.r2; }

    float contactBridgeField(const Vec3& p, float d1, float d2) const {
        const float dn = neckBridgeField(p);
        const float leftLo = neckStartX_ - neckLeftTransition_;
        const float leftHi = neckStartX_ + neckLeftTransition_;
        const float rightLo = neckEndX_ - neckRightTransition_;
        const float rightHi = neckEndX_ + neckRightTransition_;

        if (p.x <= leftLo) return d1;
        if (p.x < leftHi) {
            const float u = smootherstep((p.x - leftLo) / std::max(leftHi - leftLo, 1.0e-6f));
            return mixf(d1, dn, u);
        }
        if (p.x <= rightLo) return dn;
        if (p.x < rightHi) {
            const float u = smootherstep((p.x - rightLo) / std::max(rightHi - rightLo, 1.0e-6f));
            return mixf(dn, d2, u);
        }
        return d2;
    }


    Vec3 surfaceDirectionForWorldPoint(const Vec3& p, int lobe) const {
        const float radius = radiusForLobe(lobe);
        const LobeShape& shape = c_.shape[lobe];
        const Vec3 centre = lobe == 0 ? layout_.c1 : layout_.c2;
        const Vec3 q = worldToLocal(p - centre, lobe);
        const Vec3 normalisedPoint = {
            q.x / std::max(radius * shape.axisX, 1.0e-6f),
            q.y / std::max(radius * shape.axisY, 1.0e-6f),
            q.z / std::max(radius * shape.axisZ, 1.0e-6f)
        };
        return length(normalisedPoint) > 1.0e-8f ? normalise(normalisedPoint) : Vec3{0.0f, 1.0f, 0.0f};
    }

    SurfaceColourData directionalSurfaceData(const Vec3& direction, int lobe) const {
        SurfaceColourData data;
        data.lobe = lobe;
        data.lobeBlend = static_cast<float>(lobe);
        const float radius = radiusForLobe(lobe);
        const LobeShape& shape = c_.shape[lobe];

        if (shape.macro > 0.0f) {
            const Vec3 npos = direction * 1.65f + Vec3{3.1f * lobe, -1.7f * lobe, 2.3f * lobe};
            data.macro = fbm(npos, c_.seed + 1009 + lobe * 7919, 4) * shape.macro * 0.62f;
        }
        if (shape.roughness > 0.0f) {
            const Vec3 npos = direction * 8.0f + Vec3{-2.2f * lobe, 4.3f * lobe, 1.4f * lobe};
            data.roughness = fbm(npos, c_.seed + 2713 + lobe * 6151, 5) * shape.roughness * 0.42f;
        }

        for (int index : craterIndex_[lobe].query(direction)) {
            const Crater& crater = craters_[lobe][index];
            const float cosine = clampf(dot(direction, crater.direction), -1.0f, 1.0f);
            const float chordSq = std::max(0.0f, 2.0f - 2.0f * cosine);
            if (chordSq >= crater.supportChordSq) continue;
            const float x = std::sqrt(chordSq) * crater.invChordRadius;
            if (x < 1.0f) {
                const float u = std::max(0.0f, 1.0f - x * x);
                data.craterBowl += crater.depth * std::pow(u, 1.4f + crater.softness * 2.1f) / radius;
            }
            if (crater.rimHeight > 0.0f && x < 1.7f) {
                const float ringWidth = mixf(0.20f, 0.10f, crater.softness);
                const float ringQ = (x - 1.0f) / ringWidth;
                data.craterRim += crater.rimHeight * std::exp(-0.5f * ringQ * ringQ) / radius;
            }
        }

        for (int index : boulderIndex_[lobe].query(direction)) {
            const Boulder& boulder = boulders_[lobe][index];
            const float cosine = clampf(dot(direction, boulder.direction), -1.0f, 1.0f);
            const float chordSq = std::max(0.0f, 2.0f - 2.0f * cosine);
            if (chordSq >= boulder.supportChordSq) continue;
            const float x = std::sqrt(chordSq) * boulder.invChordRadius;
            data.boulder += boulder.height * std::pow(std::max(0.0f, 1.0f - x * x), mixf(0.6f, 2.5f, boulder.sharpness)) / radius;
        }

        for (int index : facetIndex_[lobe].query(direction)) {
            const Facet& facet = facets_[lobe][index];
            const float cosine = dot(direction, facet.direction);
            if (cosine <= facet.supportCos) continue;
            const float u = (cosine - facet.supportCos) / std::max(1.0e-6f, 1.0f - facet.supportCos);
            data.facet += facet.strength * u * u;
        }

        for (int index : grooveIndex_[lobe].query(direction)) {
            const Groove& groove = grooves_[lobe][index];
            if (dot(direction, groove.centre) <= groove.supportCos) continue;
            const float u = dot(direction, groove.tangent);
            const float v = dot(direction, groove.bitangent);
            const float lengthAbs = std::fabs(u);
            if (lengthAbs > groove.halfLength * 1.15f) continue;
            const float widthTerm = std::exp(-0.5f * (v / groove.width) * (v / groove.width));
            const float lt = clampf(1.0f - lengthAbs / std::max(groove.halfLength, 1.0e-5f), 0.0f, 1.0f);
            data.groove += (groove.depth / radius) * widthTerm * (lt * lt * (3.0f - 2.0f * lt));
        }

        const float total = data.macro + data.roughness - data.craterBowl + data.craterRim + data.boulder - data.facet - data.groove;
        const float maxInward = std::min({shape.axisX, shape.axisY, shape.axisZ}) * 0.82f;
        data.totalShift = std::max(total, -maxInward);
        return data;
    }

    SurfaceColourData blendSurfaceData(const SurfaceColourData& a, const SurfaceColourData& b, float t) const {
        t = smootherstep(t);
        SurfaceColourData out;
        out.lobe = t < 0.5f ? a.lobe : b.lobe;
        out.lobeBlend = mixf(a.lobeBlend, b.lobeBlend, t);
        out.macro = mixf(a.macro, b.macro, t);
        out.roughness = mixf(a.roughness, b.roughness, t);
        out.craterBowl = mixf(a.craterBowl, b.craterBowl, t);
        out.craterRim = mixf(a.craterRim, b.craterRim, t);
        out.boulder = mixf(a.boulder, b.boulder, t);
        out.facet = mixf(a.facet, b.facet, t);
        out.groove = mixf(a.groove, b.groove, t);
        out.totalShift = mixf(a.totalShift, b.totalShift, t);
        return out;
    }

    float neckInteriorWeight(float t) const {
        const float x = clampf(4.0f * t * (1.0f - t), 0.0f, 1.0f);
        return smootherstep(x);
    }

    float neckTextureWeight(float t) const {
        if (t <= 0.5f) return bridgeBlendWeight(t * 2.0f);
        return bridgeBlendWeight((1.0f - t) * 2.0f);
    }

    SurfaceColourData neckSurfaceData(const Vec3& p, float t, bool appliedOnly) const {
        const float blend = smootherstep(clampf(t, 0.0f, 1.0f));
        const SurfaceColourData left = directionalSurfaceData(surfaceDirectionForWorldPoint(p, 0), 0);
        const SurfaceColourData right = directionalSurfaceData(surfaceDirectionForWorldPoint(p, 1), 1);
        const float interior = neckInteriorWeight(t);
        const float surfaceFactor = appliedOnly ? mixf(1.0f, c_.neck.surfaceInheritance, interior) : 1.0f;
        const float featureFactor = appliedOnly ? mixf(1.0f, c_.neck.featureCarryover, interior) : 1.0f;

        SurfaceColourData data;
        data.lobe = t < 0.5f ? 0 : 1;
        data.lobeBlend = blend;
        data.macro = mixf(left.macro, right.macro, blend) * surfaceFactor;
        data.roughness = mixf(left.roughness, right.roughness, blend) * surfaceFactor;
        data.craterBowl = mixf(left.craterBowl, right.craterBowl, blend) * featureFactor;
        data.craterRim = mixf(left.craterRim, right.craterRim, blend) * featureFactor;
        data.boulder = mixf(left.boulder, right.boulder, blend) * featureFactor;
        data.facet = mixf(left.facet, right.facet, blend) * featureFactor;
        data.groove = mixf(left.groove, right.groove, blend) * featureFactor;

        const float dy = p.y - neckCentreY_;
        const float dz = p.z - neckCentreZ_;
        const float angle = std::atan2(dz, dy);
        const float circumferenceRadius = std::max(std::sqrt(dy * dy + dz * dz), 1.0e-5f);
        const float scaleRadius = mixf(layout_.r1, layout_.r2, blend);
        const SurfaceColourData local = neckLocalFeatureData(p.x, angle, circumferenceRadius, scaleRadius);
        const float localWeight = featureFactor * interior;
        data.craterBowl += local.craterBowl * localWeight;
        data.craterRim += local.craterRim * localWeight;
        data.boulder += local.boulder * localWeight;

        data.totalShift = data.macro + data.roughness - data.craterBowl + data.craterRim + data.boulder - data.facet - data.groove;
        return data;
    }

    float neckSurfaceShift(const Vec3& p, float t, float baseRadius) const {
        const SurfaceColourData data = neckSurfaceData(p, t, true);
        const float scaleRadius = mixf(layout_.r1, layout_.r2, smoothstep(clampf(t, 0.0f, 1.0f)));
        float shift = data.totalShift * scaleRadius;
        const float maxOutward = std::max(baseRadius * 0.32f, 0.002f * std::min(layout_.r1, layout_.r2));
        const float maxInward = std::max(baseRadius * 0.26f, 0.002f * std::min(layout_.r1, layout_.r2));
        return clampf(shift, -maxInward, maxOutward);
    }

    float contourValue(const std::array<float, NECK_CONTOUR_SAMPLES>& contour, float angle) const {
        float a = std::fmod(angle, 2.0f * PI);
        if (a < 0.0f) a += 2.0f * PI;
        const float f = a * static_cast<float>(NECK_CONTOUR_SAMPLES) / (2.0f * PI);
        const int i0 = static_cast<int>(std::floor(f)) % NECK_CONTOUR_SAMPLES;
        const int i1 = (i0 + 1) % NECK_CONTOUR_SAMPLES;
        return mixf(contour[i0], contour[i1], f - std::floor(f));
    }

    float throatRadiusAtAngle(float angle) const {
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        const float denom = std::sqrt(
            (c * c) / std::max(neckThroatY_ * neckThroatY_, 1.0e-8f) +
            (s * s) / std::max(neckThroatZ_ * neckThroatZ_, 1.0e-8f)
        );
        return 1.0f / std::max(denom, 1.0e-8f);
    }

    float bridgeBlendWeight(float u) const {
        u = clampf(u, 0.0f, 1.0f);
        const float profileT = clampf((c_.neck.profile - 0.30f) / 3.70f, 0.0f, 1.0f);
        const float profileExponent = mixf(0.60f, 2.80f, profileT);
        const float shaped = std::pow(u, profileExponent);
        return smoothstep(clampf(shaped, 0.0f, 1.0f));
    }

    float neckShoulderScale() const {
        return 1.0f + 0.40f * c_.neck.flare;
    }

    float neckShoulderTarget(float angle) const {
        return throatRadiusAtAngle(angle) * neckShoulderScale();
    }

    float neckBaseRadius(float t, float angle) const {
        const float leftRaw = contourValue(neckLeftRaw_, angle);
        const float rightRaw = contourValue(neckRightRaw_, angle);
        const float leftSmooth = contourValue(neckLeftSmooth_, angle);
        const float rightSmooth = contourValue(neckRightSmooth_, angle);
        const float throat = throatRadiusAtAngle(angle);
        const float requestedShoulder = neckShoulderTarget(angle);
        const float smoothAmount = clampf(c_.neck.smoothing / 0.20f, 0.0f, 1.0f);

        if (t <= 0.5f) {
            const float u = clampf(t * 2.0f, 0.0f, 1.0f);
            const float angularRelax = smoothstep(clampf(u * 4.0f, 0.0f, 1.0f));
            const float available = mixf(leftRaw, leftSmooth, smoothAmount * angularRelax);
            const float shoulder = std::min(available * 0.98f, requestedShoulder);
            return mixf(shoulder, throat, bridgeBlendWeight(u));
        }

        const float u = clampf((1.0f - t) * 2.0f, 0.0f, 1.0f);
        const float angularRelax = smoothstep(clampf(u * 4.0f, 0.0f, 1.0f));
        const float available = mixf(rightRaw, rightSmooth, smoothAmount * angularRelax);
        const float shoulder = std::min(available * 0.98f, requestedShoulder);
        return mixf(shoulder, throat, bridgeBlendWeight(u));
    }

    float wrappedAngleDifference(float a, float b) const {
        float d = std::fmod(a - b + PI, 2.0f * PI);
        if (d < 0.0f) d += 2.0f * PI;
        return d - PI;
    }

    SurfaceColourData neckLocalFeatureData(float x, float angle, float circumferenceRadius, float normalisationRadius) const {
        SurfaceColourData data;
        const float safeRadius = std::max(circumferenceRadius, 1.0e-5f);
        const float normaliser = std::max(normalisationRadius, 1.0e-5f);

        for (const NeckCrater& crater : neckCraters_) {
            const float dx = x - crater.x;
            const float da = wrappedAngleDifference(angle, crater.angle) * safeRadius;
            const float distance = std::sqrt(dx * dx + da * da);
            const float u = distance / std::max(crater.radius, 1.0e-5f);
            if (u < 1.0f) {
                const float bowlBase = std::max(0.0f, 1.0f - u * u);
                data.craterBowl += crater.depth * std::pow(bowlBase, 1.4f + crater.softness * 2.1f) / normaliser;
            }
            if (u < 1.7f && crater.rimHeight > 0.0f) {
                const float ringWidth = mixf(0.20f, 0.10f, crater.softness);
                const float q = (u - 1.0f) / ringWidth;
                data.craterRim += crater.rimHeight * std::exp(-0.5f * q * q) / normaliser;
            }
        }

        for (const NeckBoulder& boulder : neckBoulders_) {
            const float dx = x - boulder.x;
            const float da = wrappedAngleDifference(angle, boulder.angle) * safeRadius;
            const float distance = std::sqrt(dx * dx + da * da);
            const float u = distance / std::max(boulder.radius, 1.0e-5f);
            if (u >= 1.0f) continue;
            data.boulder += boulder.height * std::pow(std::max(0.0f, 1.0f - u * u), mixf(0.6f, 2.5f, boulder.sharpness)) / normaliser;
        }

        data.totalShift = -data.craterBowl + data.craterRim + data.boulder;
        return data;
    }

    float neckBridgeField(const Vec3& p) const {
        const float span = std::max(neckEndX_ - neckStartX_, 1.0e-5f);
        const float t = clampf((p.x - neckStartX_) / span, 0.0f, 1.0f);
        const float dy = p.y - neckCentreY_;
        const float dz = p.z - neckCentreZ_;
        const float rho = std::sqrt(dy * dy + dz * dz);
        const float angle = std::atan2(dz, dy);

        float radius = neckBaseRadius(t, angle);
        const Vec3 baseSurfacePoint{
            p.x,
            neckCentreY_ + radius * std::cos(angle),
            neckCentreZ_ + radius * std::sin(angle)
        };
        radius += neckSurfaceShift(baseSurfacePoint, t, radius);

        return rho - std::max(radius, 1.0e-5f);
    }

    float shoulderFitRatio(int lobe, float x) const {
        float ratio = 1.0e9f;
        constexpr int samples = 48;
        for (int i = 0; i < samples; ++i) {
            const float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(samples);
            const float target = std::max(neckShoulderTarget(angle), 1.0e-5f);
            const float available = sampleShoulderRadius(lobe, x, angle);
            ratio = std::min(ratio, available / target);
        }
        return ratio;
    }

    float findShoulderPlaneForNeck(
        int lobe,
        float facingSurfaceX,
        float sign,
        float requestedBurial,
        float& fitRatio
    ) const {
        const float radius = radiusForLobe(lobe);
        const Vec3 centre = lobe == 0 ? layout_.c1 : layout_.c2;
        const float step = std::max(radius * 0.01f, 0.05f);
        const float centreDepth = std::fabs(facingSurfaceX - centre.x);
        const float maxDepth = std::max(requestedBurial, centreDepth);
        float bestX = facingSurfaceX - sign * std::max(requestedBurial, 0.0f);
        float bestRatio = 0.0f;

        for (float depth = std::max(requestedBurial, 0.0f); depth <= maxDepth + 0.5f * step; depth += step) {
            const float x = facingSurfaceX - sign * depth;
            const float ratio = shoulderFitRatio(lobe, x);
            if (ratio > bestRatio) {
                bestRatio = ratio;
                bestX = x;
            }
            if (ratio >= 1.04f) {
                fitRatio = ratio;
                return x;
            }
        }

        fitRatio = bestRatio;
        return bestX;
    }

    float findShoulderPlane(int lobe, float facingSurfaceX, float sign, float requestedBurial) const {
        const float radius = radiusForLobe(lobe);
        const Vec3 centre = lobe == 0 ? layout_.c1 : layout_.c2;
        const float step = std::max(radius * 0.0125f, 0.05f);
        const float maxDepth = radius * 1.8f;
        const float margin = std::max(radius * 0.02f, 0.05f);

        for (float depth = std::max(requestedBurial, 0.0f); depth <= maxDepth; depth += step) {
            const float x = facingSurfaceX - sign * depth;
            const Vec3 centrePoint{x, neckCentreY_, neckCentreZ_};
            if (lobeSurfaceSdf(centrePoint, centre, radius, lobe) < -margin) return x;
        }
        return facingSurfaceX - sign * std::max(requestedBurial, radius * 0.30f);
    }

    float sampleShoulderRadius(int lobe, float x, float angle) const {
        const Vec3 centre = lobe == 0 ? layout_.c1 : layout_.c2;
        const float radius = radiusForLobe(lobe);
        const float ca = std::cos(angle);
        const float sa = std::sin(angle);
        const float maxRadius = std::max({bounds_[lobe].y, bounds_[lobe].z, radius}) * 1.6f;
        const Vec3 origin{x, neckCentreY_, neckCentreZ_};

        const float centreValue = lobeBaseSdf(origin, centre, radius, lobe);
        if (centreValue >= 0.0f) return 0.0f;

        constexpr int scanSteps = 80;
        float previousR = 0.0f;
        float previousValue = centreValue;
        for (int i = 1; i <= scanSteps; ++i) {
            const float r = maxRadius * static_cast<float>(i) / static_cast<float>(scanSteps);
            const Vec3 p{x, neckCentreY_ + r * ca, neckCentreZ_ + r * sa};
            const float value = lobeBaseSdf(p, centre, radius, lobe);
            if (previousValue <= 0.0f && value > 0.0f) {
                float lo = previousR;
                float hi = r;
                for (int iteration = 0; iteration < 24; ++iteration) {
                    const float mid = 0.5f * (lo + hi);
                    const Vec3 mp{x, neckCentreY_ + mid * ca, neckCentreZ_ + mid * sa};
                    if (lobeBaseSdf(mp, centre, radius, lobe) <= 0.0f) lo = mid;
                    else hi = mid;
                }
                return 0.5f * (lo + hi);
            }
            previousR = r;
            previousValue = value;
        }
        return maxRadius;
    }

    void circularSmoothContour(
        const std::array<float, NECK_CONTOUR_SAMPLES>& source,
        std::array<float, NECK_CONTOUR_SAMPLES>& destination
    ) const {
        const float amount = clampf(c_.neck.smoothing / 0.20f, 0.0f, 1.0f);
        const int radius = 1 + static_cast<int>(std::round(amount * 7.0f));
        if (amount <= 1.0e-6f) {
            destination = source;
            return;
        }

        for (int i = 0; i < NECK_CONTOUR_SAMPLES; ++i) {
            float sum = 0.0f;
            float weightSum = 0.0f;
            for (int offset = -radius; offset <= radius; ++offset) {
                const int index = (i + offset + NECK_CONTOUR_SAMPLES) % NECK_CONTOUR_SAMPLES;
                const float weight = static_cast<float>(radius + 1 - std::abs(offset));
                sum += source[index] * weight;
                weightSum += weight;
            }
            destination[i] = sum / std::max(weightSum, 1.0e-6f);
        }
    }

    void buildNeckContours() {
        for (int i = 0; i < NECK_CONTOUR_SAMPLES; ++i) {
            const float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(NECK_CONTOUR_SAMPLES);
            neckLeftRaw_[i] = sampleShoulderRadius(0, neckStartX_, angle);
            neckRightRaw_[i] = sampleShoulderRadius(1, neckEndX_, angle);
        }
        circularSmoothContour(neckLeftRaw_, neckLeftSmooth_);
        circularSmoothContour(neckRightRaw_, neckRightSmooth_);
    }


    void buildNeckSurfaceFeatures() {
        neckCraters_.clear();
        neckBoulders_.clear();
        if (c_.topology != "contact") return;

        const float span = std::max(neckEndX_ - neckStartX_, 1.0e-5f);
        float meanRadius = 0.0f;
        constexpr int radiusSamples = 24;
        for (int i = 0; i < radiusSamples; ++i) {
            const float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(radiusSamples);
            meanRadius += neckBaseRadius(0.5f, angle);
        }
        meanRadius /= static_cast<float>(radiusSamples);
        meanRadius = std::max(meanRadius, 0.02f * std::min(layout_.r1, layout_.r2));

        const float neckArea = 2.0f * PI * meanRadius * span * 1.20f;
        const float area0 = 4.0f * PI * layout_.r1 * layout_.r1;
        const float area1 = 4.0f * PI * layout_.r2 * layout_.r2;
        const float craterDensity = 0.5f * (
            static_cast<float>(c_.crater[0].count) / std::max(area0, 1.0e-5f) +
            static_cast<float>(c_.crater[1].count) / std::max(area1, 1.0e-5f)
        );
        const float boulderDensity = 0.5f * (
            static_cast<float>(c_.boulder[0].count) / std::max(area0, 1.0e-5f) +
            static_cast<float>(c_.boulder[1].count) / std::max(area1, 1.0e-5f)
        );

        const int craterCount = std::clamp(static_cast<int>(std::lround(neckArea * craterDensity)), 0, 320);
        const int boulderCount = std::clamp(static_cast<int>(std::lround(neckArea * boulderDensity)), 0, 320);
        uint32_t state = hash32(static_cast<uint32_t>(c_.seed) ^ 0x6e65636bu);

        neckCraters_.reserve(static_cast<size_t>(craterCount));
        for (int i = 0; i < craterCount; ++i) {
            const float t = mixf(0.10f, 0.90f, rand01(state));
            const float blend = smoothstep(t);
            const float angle = rand01(state) * 2.0f * PI;
            const float baseRadius = std::max(neckBaseRadius(t, angle), 1.0e-5f);
            const CraterSettings settings{
                0,
                mixf(c_.crater[0].minRadius, c_.crater[1].minRadius, blend),
                mixf(c_.crater[0].maxRadius, c_.crater[1].maxRadius, blend),
                mixf(c_.crater[0].depth, c_.crater[1].depth, blend),
                mixf(c_.crater[0].rim, c_.crater[1].rim, blend),
                mixf(c_.crater[0].exponent, c_.crater[1].exponent, blend),
                mixf(c_.crater[0].freshness, c_.crater[1].freshness, blend)
            };
            const float scaleRadius = mixf(layout_.r1, layout_.r2, blend);
            const float fraction = mixf(settings.minRadius, settings.maxRadius, std::pow(rand01(state), settings.exponent));
            const float mouthRadius = std::min(scaleRadius * fraction, baseRadius * 0.42f);
            if (mouthRadius <= 1.0e-4f) continue;
            const float randomAge = rand01(state);
            const float preservation = clampf(mixf(0.25f, 1.0f, settings.freshness) * (0.55f + 0.45f * (1.0f - randomAge)), 0.15f, 1.0f);

            NeckCrater crater;
            crater.x = mixf(neckStartX_, neckEndX_, t);
            crater.angle = angle;
            crater.radius = mouthRadius;
            crater.depth = mouthRadius * settings.depth * preservation;
            crater.rimHeight = mouthRadius * settings.rim * 0.10f * preservation;
            crater.softness = preservation;
            neckCraters_.push_back(crater);
        }

        neckBoulders_.reserve(static_cast<size_t>(boulderCount));
        for (int i = 0; i < boulderCount; ++i) {
            const float t = mixf(0.10f, 0.90f, rand01(state));
            const float blend = smoothstep(t);
            const float angle = rand01(state) * 2.0f * PI;
            const float baseRadius = std::max(neckBaseRadius(t, angle), 1.0e-5f);
            const float minFraction = mixf(c_.boulder[0].minRadius, c_.boulder[1].minRadius, blend);
            const float maxFraction = mixf(c_.boulder[0].maxRadius, c_.boulder[1].maxRadius, blend);
            const float scaleRadius = mixf(layout_.r1, layout_.r2, blend);
            const float moundRadius = std::min(scaleRadius * mixf(minFraction, maxFraction, std::pow(rand01(state), 1.6f)), baseRadius * 0.34f);
            if (moundRadius <= 1.0e-4f) continue;

            NeckBoulder boulder;
            boulder.x = mixf(neckStartX_, neckEndX_, t);
            boulder.angle = angle;
            boulder.radius = moundRadius;
            boulder.height = moundRadius * mixf(c_.boulder[0].height, c_.boulder[1].height, blend) * mixf(0.5f, 1.0f, rand01(state));
            boulder.sharpness = clampf(mixf(c_.boulder[0].sharpness, c_.boulder[1].sharpness, blend) * 0.75f + 0.25f * rand01(state), 0.0f, 1.0f);
            neckBoulders_.push_back(boulder);
        }
    }

    void buildBases() {
        for (int lobe = 0; lobe < 2; ++lobe) {
            const LobeShape& s = c_.shape[lobe];
            const float rx = s.rotationX * PI / 180.0f;
            const float ry = s.rotationY * PI / 180.0f;
            const float rz = s.rotationZ * PI / 180.0f;
            const float cx = std::cos(rx), sx = std::sin(rx);
            const float cy = std::cos(ry), sy = std::sin(ry);
            const float cz = std::cos(rz), sz = std::sin(rz);

            basis_[lobe].x = {
                cz * cy,
                sz * cy,
                -sy
            };
            basis_[lobe].y = {
                cz * sy * sx - sz * cx,
                sz * sy * sx + cz * cx,
                cy * sx
            };
            basis_[lobe].z = {
                cz * sy * cx + sz * sx,
                sz * sy * cx - cz * sx,
                cy * cx
            };
        }
    }

    Vec3 localToWorld(const Vec3& v, int lobe) const {
        const LobeBasis& b = basis_[lobe];
        return b.x * v.x + b.y * v.y + b.z * v.z;
    }

    Vec3 worldToLocal(const Vec3& v, int lobe) const {
        const LobeBasis& b = basis_[lobe];
        return {dot(v, b.x), dot(v, b.y), dot(v, b.z)};
    }

    float directionalShapeScale(const Vec3& direction, int lobe) const {
        const LobeShape& s = c_.shape[lobe];
        const float taperTerm = -s.taper * direction.x;
        const float asymTerm = s.asymmetry * direction.x * std::fabs(direction.x);
        return clampf(1.0f + taperTerm + asymTerm, 0.35f, 1.95f);
    }

    Vec3 surfacePointForDirection(const Vec3& direction, float radius, int lobe) const {
        const LobeShape& s = c_.shape[lobe];
        const float equator = std::exp(-0.5f * (direction.y / 0.30f) * (direction.y / 0.30f));
        const float ridgeScale = 1.0f + s.ridge * equator;
        const float scale = directionalShapeScale(direction, lobe);
        return {
            radius * s.axisX * ridgeScale * scale * direction.x,
            radius * s.axisY * scale * direction.y,
            radius * s.axisZ * ridgeScale * scale * direction.z
        };
    }

    float surfaceAreaJacobian(const Vec3& direction, float radius, int lobe) const {
        Vec3 reference = std::fabs(direction.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
        Vec3 tangent1 = cross(reference, direction);
        if (length(tangent1) <= 1.0e-8f) tangent1 = cross(Vec3{0.0f, 0.0f, 1.0f}, direction);
        tangent1 = normalise(tangent1);
        const Vec3 tangent2 = cross(direction, tangent1);
        constexpr float eps = 1.0e-3f;
        const Vec3 u1p = normalise(direction + tangent1 * eps);
        const Vec3 u1m = normalise(direction - tangent1 * eps);
        const Vec3 u2p = normalise(direction + tangent2 * eps);
        const Vec3 u2m = normalise(direction - tangent2 * eps);
        const Vec3 d1 = (surfacePointForDirection(u1p, radius, lobe) - surfacePointForDirection(u1m, radius, lobe)) / (2.0f * eps);
        const Vec3 d2 = (surfacePointForDirection(u2p, radius, lobe) - surfacePointForDirection(u2m, radius, lobe)) / (2.0f * eps);
        return std::max(length(cross(d1, d2)), 1.0e-8f);
    }

    float surfaceAreaUpperBound(float radius, int lobe) const {
        const LobeShape& s = c_.shape[lobe];
        const float maxAxis = std::max({s.axisX, s.axisY, s.axisZ});
        const float bias = 1.0f + s.ridge + std::fabs(s.taper) + std::fabs(s.asymmetry);
        return std::max(radius * radius * maxAxis * maxAxis * bias * bias * 1.25f, 1.0e-8f);
    }

    Vec3 randomSurfaceDirection(uint32_t& state, float radius, int lobe) const {
        const float maxWeight = surfaceAreaUpperBound(radius, lobe);
        for (int attempt = 0; attempt < 4096; ++attempt) {
            const Vec3 candidate = randomUnitVector(state);
            if (rand01(state) * maxWeight <= surfaceAreaJacobian(candidate, radius, lobe)) return candidate;
        }
        return randomUnitVector(state);
    }

    void buildFeatures(int lobe) {
        const float radius = radiusForLobe(lobe);
        buildCraters(lobe, radius);
        buildBoulders(lobe, radius);
        buildFacets(lobe, radius);
        buildGrooves(lobe, radius);
    }

    void buildCraters(int lobe, float radius) {
        const CraterSettings& settings = c_.crater[lobe];
        if (settings.count <= 0) return;
        uint32_t state = hash32(static_cast<uint32_t>(c_.seed) ^ (lobe == 0 ? 0xa511e9b3u : 0x713ac4d1u));
        craters_[lobe].reserve(static_cast<size_t>(settings.count));
        for (int i = 0; i < settings.count; ++i) {
            const Vec3 direction = randomSurfaceDirection(state, radius, lobe);
            const float fraction = mixf(settings.minRadius, settings.maxRadius, std::pow(rand01(state), settings.exponent));
            const float mouthRadius = radius * fraction;
            const float localScale = std::sqrt(surfaceAreaJacobian(direction, radius, lobe));
            const float angularRadius = clampf(mouthRadius / std::max(localScale, 1.0e-5f), 0.002f, 0.78f);
            const float age = rand01(state);
            const float preservation = clampf(mixf(0.22f, 1.0f, settings.freshness) * (0.55f + 0.45f * (1.0f - age)), 0.12f, 1.0f);

            Crater crater;
            crater.direction = direction;
            crater.depth = std::min(mouthRadius * settings.depth * preservation, radius * 0.34f);
            crater.rimHeight = mouthRadius * settings.rim * 0.10f * preservation;
            crater.softness = preservation;
            const float chord = std::sqrt(std::max(1.0e-8f, 2.0f - 2.0f * std::cos(angularRadius)));
            crater.invChordRadius = 1.0f / chord;
            crater.supportAngle = std::min(PI, angularRadius * (crater.rimHeight > 0.0f ? 1.8f : 1.2f));
            crater.supportChordSq = std::max(1.0e-8f, 2.0f - 2.0f * std::cos(crater.supportAngle));
            craters_[lobe].push_back(crater);
        }
    }

    void buildBoulders(int lobe, float radius) {
        const BoulderSettings& settings = c_.boulder[lobe];
        if (settings.count <= 0) return;
        uint32_t state = hash32(static_cast<uint32_t>(c_.seed) ^ (lobe == 0 ? 0x43f9a21du : 0xd15ea5e1u));
        boulders_[lobe].reserve(static_cast<size_t>(settings.count));
        for (int i = 0; i < settings.count; ++i) {
            const Vec3 direction = randomSurfaceDirection(state, radius, lobe);
            const float fraction = mixf(settings.minRadius, settings.maxRadius, std::pow(rand01(state), 1.6f));
            const float moundRadius = radius * fraction;
            const float localScale = std::sqrt(surfaceAreaJacobian(direction, radius, lobe));
            const float angularRadius = clampf(moundRadius / std::max(localScale, 1.0e-5f), 0.0015f, 0.38f);

            Boulder boulder;
            boulder.direction = direction;
            boulder.height = moundRadius * settings.height * mixf(0.5f, 1.0f, rand01(state));
            boulder.sharpness = clampf(settings.sharpness * 0.75f + 0.25f * rand01(state), 0.0f, 1.0f);
            const float chord = std::sqrt(std::max(1.0e-8f, 2.0f - 2.0f * std::cos(angularRadius)));
            boulder.invChordRadius = 1.0f / chord;
            boulder.supportAngle = std::min(PI, angularRadius * 1.35f);
            boulder.supportChordSq = std::max(1.0e-8f, 2.0f - 2.0f * std::cos(boulder.supportAngle));
            boulders_[lobe].push_back(boulder);
        }
    }

    void buildFacets(int lobe, float radius) {
        const TerrainSettings& settings = c_.terrain[lobe];
        if (settings.facetCount <= 0 || settings.facetStrength <= 0.0f) return;
        uint32_t state = hash32(static_cast<uint32_t>(c_.seed) ^ (lobe == 0 ? 0x19abce07u : 0x9a55c713u));
        facets_[lobe].reserve(static_cast<size_t>(settings.facetCount));
        for (int i = 0; i < settings.facetCount; ++i) {
            Facet facet;
            facet.direction = randomSurfaceDirection(state, radius, lobe);
            facet.supportCos = mixf(0.75f, 0.94f, rand01(state));
            facet.supportAngle = std::acos(facet.supportCos);
            facet.strength = settings.facetStrength * mixf(0.25f, 1.0f, rand01(state)) * 0.22f;
            facets_[lobe].push_back(facet);
        }
    }

    void buildGrooves(int lobe, float radius) {
        const TerrainSettings& settings = c_.terrain[lobe];
        if (settings.grooveCount <= 0 || settings.grooveDepth <= 0.0f) return;
        uint32_t state = hash32(static_cast<uint32_t>(c_.seed) ^ (lobe == 0 ? 0xc001d00du : 0x51ec7a11u));
        grooves_[lobe].reserve(static_cast<size_t>(settings.grooveCount));
        for (int i = 0; i < settings.grooveCount; ++i) {
            const Vec3 centre = randomSurfaceDirection(state, radius, lobe);
            Vec3 tangent = cross(std::fabs(centre.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f}, centre);
            tangent = normalise(tangent);
            const float angle = rand01(state) * 2.0f * PI;
            const Vec3 bitangent0 = normalise(cross(centre, tangent));
            const Vec3 orientedTangent = normalise(tangent * std::cos(angle) + bitangent0 * std::sin(angle));
            const Vec3 orientedBitangent = normalise(cross(centre, orientedTangent));

            Groove groove;
            groove.centre = centre;
            groove.tangent = orientedTangent;
            groove.bitangent = orientedBitangent;
            groove.width = settings.grooveWidth * mixf(0.7f, 1.3f, rand01(state));
            groove.halfLength = settings.grooveLength * mixf(0.7f, 1.3f, rand01(state));
            groove.depth = radius * settings.grooveDepth * 0.18f * mixf(0.6f, 1.0f, rand01(state));
            groove.supportAngle = std::min(1.30f, std::sqrt(groove.width * groove.width + groove.halfLength * groove.halfLength) * 1.35f);
            groove.supportCos = std::cos(groove.supportAngle);
            grooves_[lobe].push_back(groove);
        }
    }

    void buildIndices() {
        for (int lobe = 0; lobe < 2; ++lobe) {
            craterIndex_[lobe].clear();
            boulderIndex_[lobe].clear();
            facetIndex_[lobe].clear();
            grooveIndex_[lobe].clear();
            for (int i = 0; i < static_cast<int>(craters_[lobe].size()); ++i) craterIndex_[lobe].add(i, craters_[lobe][i].direction, craters_[lobe][i].supportAngle);
            for (int i = 0; i < static_cast<int>(boulders_[lobe].size()); ++i) boulderIndex_[lobe].add(i, boulders_[lobe][i].direction, boulders_[lobe][i].supportAngle);
            for (int i = 0; i < static_cast<int>(facets_[lobe].size()); ++i) facetIndex_[lobe].add(i, facets_[lobe][i].direction, facets_[lobe][i].supportAngle);
            for (int i = 0; i < static_cast<int>(grooves_[lobe].size()); ++i) grooveIndex_[lobe].add(i, grooves_[lobe][i].centre, grooves_[lobe][i].supportAngle);
        }
    }

    float craterSurfaceShift(const Vec3& direction, int lobe) const {
        float shift = 0.0f;
        for (int index : craterIndex_[lobe].query(direction)) {
            const Crater& crater = craters_[lobe][index];
            const float cosine = clampf(dot(direction, crater.direction), -1.0f, 1.0f);
            const float chordSq = std::max(0.0f, 2.0f - 2.0f * cosine);
            if (chordSq >= crater.supportChordSq) continue;
            const float x = std::sqrt(chordSq) * crater.invChordRadius;
            if (x < 1.0f) {
                const float u = std::max(0.0f, 1.0f - x * x);
                shift -= crater.depth * std::pow(u, 1.4f + crater.softness * 2.1f);
            }
            if (crater.rimHeight > 0.0f && x < 1.7f) {
                const float ringWidth = mixf(0.20f, 0.10f, crater.softness);
                const float q = (x - 1.0f) / ringWidth;
                shift += crater.rimHeight * std::exp(-0.5f * q * q);
            }
        }
        return shift;
    }

    float boulderSurfaceShift(const Vec3& direction, int lobe) const {
        float shift = 0.0f;
        for (int index : boulderIndex_[lobe].query(direction)) {
            const Boulder& boulder = boulders_[lobe][index];
            const float cosine = clampf(dot(direction, boulder.direction), -1.0f, 1.0f);
            const float chordSq = std::max(0.0f, 2.0f - 2.0f * cosine);
            if (chordSq >= boulder.supportChordSq) continue;
            const float x = std::sqrt(chordSq) * boulder.invChordRadius;
            shift += boulder.height * std::pow(std::max(0.0f, 1.0f - x * x), mixf(0.6f, 2.5f, boulder.sharpness));
        }
        return shift;
    }

    float facetSurfaceShift(const Vec3& direction, int lobe, float radius) const {
        float shift = 0.0f;
        for (int index : facetIndex_[lobe].query(direction)) {
            const Facet& facet = facets_[lobe][index];
            const float cosine = dot(direction, facet.direction);
            if (cosine <= facet.supportCos) continue;
            const float t = (cosine - facet.supportCos) / std::max(1.0e-6f, 1.0f - facet.supportCos);
            shift -= facet.strength * radius * t * t;
        }
        return shift;
    }

    float grooveSurfaceShift(const Vec3& direction, int lobe) const {
        float shift = 0.0f;
        for (int index : grooveIndex_[lobe].query(direction)) {
            const Groove& groove = grooves_[lobe][index];
            if (dot(direction, groove.centre) <= groove.supportCos) continue;
            const float u = dot(direction, groove.tangent);
            const float v = dot(direction, groove.bitangent);
            const float lengthAbs = std::fabs(u);
            if (lengthAbs > groove.halfLength * 1.15f) continue;
            const float widthTerm = std::exp(-0.5f * (v / groove.width) * (v / groove.width));
            const float lt = clampf(1.0f - lengthAbs / std::max(groove.halfLength, 1.0e-5f), 0.0f, 1.0f);
            shift -= groove.depth * widthTerm * (lt * lt * (3.0f - 2.0f * lt));
        }
        return shift;
    }

    float structuralSurfaceShift(const Vec3& direction, int lobe, float radius) const {
        const LobeShape& s = c_.shape[lobe];
        float shift = 0.0f;
        if (s.macro > 0.0f) {
            const Vec3 npos = direction * 1.65f + Vec3{3.1f * lobe, -1.7f * lobe, 2.3f * lobe};
            shift += fbm(npos, c_.seed + 1009 + lobe * 7919, 4) * s.macro * radius * 0.62f;
        }
        if (s.roughness > 0.0f) {
            const Vec3 npos = direction * 8.0f + Vec3{-2.2f * lobe, 4.3f * lobe, 1.4f * lobe};
            shift += fbm(npos, c_.seed + 2713 + lobe * 6151, 5) * s.roughness * radius * 0.42f;
        }
        const float maxInward = radius * std::min({s.axisX, s.axisY, s.axisZ}) * 0.70f;
        return std::max(shift, -maxInward);
    }

    float surfaceShift(const Vec3& direction, int lobe, float radius) const {
        const LobeShape& s = c_.shape[lobe];
        float shift = structuralSurfaceShift(direction, lobe, radius);
        shift += craterSurfaceShift(direction, lobe);
        shift += boulderSurfaceShift(direction, lobe);
        shift += facetSurfaceShift(direction, lobe, radius);
        shift += grooveSurfaceShift(direction, lobe);
        const float maxInward = radius * std::min({s.axisX, s.axisY, s.axisZ}) * 0.82f;
        return std::max(shift, -maxInward);
    }

    Vec3 surfacePointLocal(const Vec3& direction, int lobe, bool structuralOnly) const {
        const float radius = radiusForLobe(lobe);
        const LobeShape& s = c_.shape[lobe];
        const float equator = std::exp(-0.5f * (direction.y / 0.30f) * (direction.y / 0.30f));
        const float ridgeScale = 1.0f + s.ridge * equator;
        const float shapeScale = directionalShapeScale(direction, lobe);
        const float rx = radius * s.axisX * ridgeScale * shapeScale;
        const float ry = radius * s.axisY * shapeScale;
        const float rz = radius * s.axisZ * ridgeScale * shapeScale;
        const float metric = std::min({rx, ry, rz});
        const float shift = structuralOnly ? structuralSurfaceShift(direction, lobe, radius) : surfaceShift(direction, lobe, radius);
        const float radialScale = std::max(0.12f, 1.0f + shift / std::max(metric, 1.0e-5f));
        return {rx * direction.x * radialScale, ry * direction.y * radialScale, rz * direction.z * radialScale};
    }

    Vec3 actualSurfacePointLocal(const Vec3& direction, int lobe) const {
        return surfacePointLocal(direction, lobe, false);
    }

    Vec3 structuralSurfacePointLocal(const Vec3& direction, int lobe) const {
        return surfacePointLocal(direction, lobe, true);
    }

    float facingExtent(int lobe, float sign, float percentile) const {
        std::vector<float> extents;
        extents.reserve(81);
        const Vec3 axisWorld{sign, 0.0f, 0.0f};
        const Vec3 axisLocal = normalise(worldToLocal(axisWorld, lobe));
        extents.push_back(sign * localToWorld(structuralSurfacePointLocal(axisLocal, lobe), lobe).x);
        for (int ring = 1; ring <= 4; ++ring) {
            const float angle = (16.0f * PI / 180.0f) * static_cast<float>(ring) / 4.0f;
            const int points = ring * 8;
            for (int j = 0; j < points; ++j) {
                const float phi = 2.0f * PI * static_cast<float>(j) / points;
                const Vec3 dirWorld = normalise(Vec3{sign * std::cos(angle), std::sin(angle) * std::cos(phi), std::sin(angle) * std::sin(phi)});
                const Vec3 dirLocal = normalise(worldToLocal(dirWorld, lobe));
                extents.push_back(sign * localToWorld(structuralSurfacePointLocal(dirLocal, lobe), lobe).x);
            }
        }
        std::sort(extents.begin(), extents.end());
        const size_t idx = static_cast<size_t>(clampf(percentile, 0.0f, 1.0f) * static_cast<float>(extents.size() - 1));
        return std::max(extents[idx], radiusForLobe(lobe) * 0.1f);
    }

    float facingAxisExtent(int lobe, float sign) const {
        const Vec3 axisWorld{sign, 0.0f, 0.0f};
        const Vec3 axisLocal = normalise(worldToLocal(axisWorld, lobe));
        return std::max(sign * localToWorld(structuralSurfacePointLocal(axisLocal, lobe), lobe).x, radiusForLobe(lobe) * 0.08f);
    }

    void finaliseLayout() {
        if (c_.topology == "single") return;
        float distance = 0.0f;
        if (c_.topology == "contact") {
            const float e1 = facingAxisExtent(0, +1.0f);
            const float e2 = facingAxisExtent(1, -1.0f);
            const float minRadius = std::min(layout_.r1, layout_.r2);
            const float gap = c_.neck.length * minRadius;
            distance = std::max(e1 + e2 + gap, 0.15f * (layout_.r1 + layout_.r2));

            const float total = layout_.r1 + layout_.r2;
            layout_.c1.x = -distance * (layout_.r2 / total);
            layout_.c2.x = distance * (layout_.r1 / total);

            const float surface1 = layout_.c1.x + e1;
            const float surface2 = layout_.c2.x - e2;
            neckCentreY_ = c_.neck.offsetY * minRadius;
            neckCentreZ_ = c_.neck.offsetZ * minRadius;
            neckThroatY_ = std::max(c_.neck.radiusY * minRadius, minRadius * 0.02f);
            neckThroatZ_ = std::max(c_.neck.radiusZ * minRadius, minRadius * 0.02f);

            const float requestedBurial = c_.neck.inset * minRadius;
            float leftFit = 0.0f;
            float rightFit = 0.0f;
            neckStartX_ = findShoulderPlaneForNeck(0, surface1, +1.0f, requestedBurial, leftFit);
            neckEndX_ = findShoulderPlaneForNeck(1, surface2, -1.0f, requestedBurial, rightFit);

            const float limitingFit = std::min(leftFit, rightFit);
            if (limitingFit < 1.0f) {
                const float scale = clampf(limitingFit / 1.04f, 0.12f, 1.0f);
                neckThroatY_ *= scale;
                neckThroatZ_ *= scale;
            }

            if (neckEndX_ <= neckStartX_ + minRadius * 0.04f) {
                const float mid = 0.5f * (surface1 + surface2);
                neckStartX_ = mid - minRadius * 0.02f;
                neckEndX_ = mid + minRadius * 0.02f;
            }
            buildNeckContours();
            const float neckSpan = std::max(neckEndX_ - neckStartX_, minRadius * 0.04f);
            const float transition = std::min(minRadius * 0.18f, neckSpan * 0.24f);
            neckLeftTransition_ = std::max(transition, minRadius * 0.035f);
            neckRightTransition_ = neckLeftTransition_;
            buildNeckSurfaceFeatures();
            return;
        }

        const float e1 = facingExtent(0, +1.0f, 0.98f);
        const float e2 = facingExtent(1, -1.0f, 0.98f);
        distance = (e1 + e2) * c_.separation;
        const float total = layout_.r1 + layout_.r2;
        layout_.c1.x = -distance * (layout_.r2 / total);
        layout_.c2.x = distance * (layout_.r1 / total);
    }

    void updateLobeBounds() {
        for (int lobe = 0; lobe < 2; ++lobe) {
            const float radius = radiusForLobe(lobe);
            const LobeShape& s = c_.shape[lobe];
            const CraterSettings& cr = c_.crater[lobe];
            const BoulderSettings& b = c_.boulder[lobe];
            const float shapeMax = 1.0f + std::fabs(s.taper) + std::fabs(s.asymmetry);
            const float outward = radius * (0.62f * s.macro + 0.42f * s.roughness + 0.10f * cr.maxRadius * cr.rim + b.maxRadius * b.height + 0.06f);
            const float localX = radius * s.axisX * (1.0f + s.ridge) * shapeMax + outward;
            const float localY = radius * s.axisY * shapeMax + outward;
            const float localZ = radius * s.axisZ * (1.0f + s.ridge) * shapeMax + outward;
            const LobeBasis& basis = basis_[lobe];
            bounds_[lobe].x = std::fabs(basis.x.x) * localX + std::fabs(basis.y.x) * localY + std::fabs(basis.z.x) * localZ;
            bounds_[lobe].y = std::fabs(basis.x.y) * localX + std::fabs(basis.y.y) * localY + std::fabs(basis.z.y) * localZ;
            bounds_[lobe].z = std::fabs(basis.x.z) * localX + std::fabs(basis.y.z) * localY + std::fabs(basis.z.z) * localZ;
        }
    }

    float lobeBaseSdf(const Vec3& p, const Vec3& centre, float radius, int lobe) const {
        const Vec3 qWorld = p - centre;
        const Vec3 q = worldToLocal(qWorld, lobe);
        const LobeShape& s = c_.shape[lobe];
        const float baseRx = radius * s.axisX;
        const float baseRy = radius * s.axisY;
        const float baseRz = radius * s.axisZ;
        const Vec3 normalisedPoint = {
            q.x / std::max(baseRx, 1.0e-6f),
            q.y / std::max(baseRy, 1.0e-6f),
            q.z / std::max(baseRz, 1.0e-6f)
        };
        const float baseEll = length(normalisedPoint);
        const Vec3 direction = baseEll > 1.0e-8f ? normalise(normalisedPoint) : Vec3{0.0f, 1.0f, 0.0f};
        const float equator = std::exp(-0.5f * (direction.y / 0.30f) * (direction.y / 0.30f));
        const float ridgeScale = 1.0f + s.ridge * equator;
        const float shapeScale = directionalShapeScale(direction, lobe);
        const float rx = baseRx * ridgeScale * shapeScale;
        const float ry = baseRy * shapeScale;
        const float rz = baseRz * ridgeScale * shapeScale;
        const float ell = std::sqrt((q.x * q.x) / (rx * rx) + (q.y * q.y) / (ry * ry) + (q.z * q.z) / (rz * rz));
        const float metric = std::min({rx, ry, rz});
        return (ell - 1.0f) * metric;
    }

    float lobeSurfaceSdf(const Vec3& p, const Vec3& centre, float radius, int lobe) const {
        const Vec3 qWorld = p - centre;
        const Vec3 q = worldToLocal(qWorld, lobe);
        const LobeShape& s = c_.shape[lobe];
        const float baseRx = radius * s.axisX;
        const float baseRy = radius * s.axisY;
        const float baseRz = radius * s.axisZ;
        const Vec3 normalisedPoint = {
            q.x / std::max(baseRx, 1.0e-6f),
            q.y / std::max(baseRy, 1.0e-6f),
            q.z / std::max(baseRz, 1.0e-6f)
        };
        const float baseEll = length(normalisedPoint);
        const Vec3 direction = baseEll > 1.0e-8f ? normalise(normalisedPoint) : Vec3{0.0f, 1.0f, 0.0f};
        const float equator = std::exp(-0.5f * (direction.y / 0.30f) * (direction.y / 0.30f));
        const float ridgeScale = 1.0f + s.ridge * equator;
        const float shapeScale = directionalShapeScale(direction, lobe);
        const float rx = baseRx * ridgeScale * shapeScale;
        const float ry = baseRy * shapeScale;
        const float rz = baseRz * ridgeScale * shapeScale;
        const float ell = std::sqrt((q.x * q.x) / (rx * rx) + (q.y * q.y) / (ry * ry) + (q.z * q.z) / (rz * rz));
        const float metric = std::min({rx, ry, rz});
        return (ell - 1.0f) * metric - surfaceShift(direction, lobe, radius);
    }
};

class Mesher {
public:
    Mesher(const AsteroidField& field, const Config& config) : field_(field), c_(config) {}

    void build(std::vector<Vec3>& vertices, std::vector<Tri>& triangles, int& nxOut, int& nyOut, int& nzOut) {
        Vec3 mn, mx;
        field_.bounds(mn, mx);
        const Vec3 ext = mx - mn;
        const float longest = std::max({ext.x, ext.y, ext.z});

        float featureLoad = 0.0f;
        for (int lobe = 0; lobe < (c_.topology == "single" ? 1 : 2); ++lobe) {
            featureLoad += c_.crater[lobe].count * std::max(0.01f, c_.crater[lobe].maxRadius);
            featureLoad += c_.boulder[lobe].count * std::max(0.01f, c_.boulder[lobe].maxRadius);
            featureLoad += c_.terrain[lobe].grooveCount * 0.08f + c_.terrain[lobe].facetCount * 0.05f;
        }
        const float autoBoost = 1.0f + c_.adaptiveDetail * std::min(0.45f, std::sqrt(std::max(0.0f, featureLoad)) / 25.0f);
        const float effectiveResolution = clampf(c_.resolution * autoBoost, 20.0f, 360.0f);

        const int nx = std::max(18, static_cast<int>(std::round(effectiveResolution * ext.x / longest)));
        const int ny = std::max(18, static_cast<int>(std::round(effectiveResolution * ext.y / longest)));
        const int nz = std::max(18, static_cast<int>(std::round(effectiveResolution * ext.z / longest)));
        nxOut = nx;
        nyOut = ny;
        nzOut = nz;

        const int sx = nx + 1;
        const int sy = ny + 1;
        const int sz = nz + 1;
        const float dx = ext.x / static_cast<float>(nx);
        const float dy = ext.y / static_cast<float>(ny);
        const float dz = ext.z / static_cast<float>(nz);
        const float scalarEpsilon = std::max(c_.size * 1.0e-7f, 1.0e-7f);

        std::vector<float> values(static_cast<size_t>(sx) * sy * sz);
        auto nodeId = [sx, sy](int x, int y, int z) -> uint32_t {
            return static_cast<uint32_t>((z * sy + y) * sx + x);
        };
        auto nodePos = [&](int x, int y, int z) -> Vec3 {
            return {mn.x + dx * x, mn.y + dy * y, mn.z + dz * z};
        };

        for (int z = 0; z <= nz; ++z) {
            for (int y = 0; y <= ny; ++y) {
                for (int x = 0; x <= nx; ++x) {
                    float value = field_.sample(nodePos(x, y, z));
                    if (std::fabs(value) < scalarEpsilon) value = value < 0.0f ? -scalarEpsilon : scalarEpsilon;
                    values[nodeId(x, y, z)] = value;
                }
            }
        }

        static constexpr int cubeCorners[8][3] = {
            {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0},
            {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}
        };
        static constexpr int tets[6][4] = {
            {0,5,1,6}, {0,1,2,6}, {0,2,3,6},
            {0,3,7,6}, {0,7,4,6}, {0,4,5,6}
        };

        std::unordered_map<uint64_t, int> edgeCache;
        edgeCache.reserve(static_cast<size_t>(nx) * ny * nz * 4);

        auto edgeVertex = [&](uint32_t idA, uint32_t idB, const Vec3& pA, const Vec3& pB, float vA, float vB) -> int {
            const uint32_t lo = std::min(idA, idB);
            const uint32_t hi = std::max(idA, idB);
            const uint64_t key = (static_cast<uint64_t>(lo) << 32) | hi;
            const auto found = edgeCache.find(key);
            if (found != edgeCache.end()) return found->second;
            float t = 0.5f;
            const float denom = vA - vB;
            if (std::fabs(denom) > 1.0e-12f) t = clampf(vA / denom, 1.0e-6f, 1.0f - 1.0e-6f);
            const int index = static_cast<int>(vertices.size());
            vertices.push_back(lerp(pA, pB, t));
            edgeCache.emplace(key, index);
            return index;
        };

        auto emit = [&](int a, int b, int cidx, const Vec3& outwardHint) {
            if (a == b || b == cidx || a == cidx) return;
            const Vec3 n = cross(vertices[b] - vertices[a], vertices[cidx] - vertices[a]);
            if (length(n) <= 1.0e-10f) return;
            Tri t{a, b, cidx};
            if (dot(n, outwardHint) < 0.0f) std::swap(t.b, t.c);
            triangles.push_back(t);
        };

        for (int z = 0; z < nz; ++z) {
            for (int y = 0; y < ny; ++y) {
                for (int x = 0; x < nx; ++x) {
                    std::array<uint32_t, 8> ids{};
                    std::array<Vec3, 8> pos{};
                    std::array<float, 8> val{};
                    for (int c = 0; c < 8; ++c) {
                        const int gx = x + cubeCorners[c][0];
                        const int gy = y + cubeCorners[c][1];
                        const int gz = z + cubeCorners[c][2];
                        ids[c] = nodeId(gx, gy, gz);
                        pos[c] = nodePos(gx, gy, gz);
                        val[c] = values[ids[c]];
                    }

                    for (const auto& tet : tets) {
                        std::array<int, 4> inside{};
                        std::array<int, 4> outside{};
                        int inCount = 0;
                        int outCount = 0;
                        for (int i = 0; i < 4; ++i) {
                            const int ci = tet[i];
                            if (val[ci] <= 0.0f) inside[inCount++] = ci;
                            else outside[outCount++] = ci;
                        }
                        if (inCount == 0 || inCount == 4) continue;

                        if (inCount == 1 || inCount == 3) {
                            const bool oneInside = inCount == 1;
                            const int pivot = oneInside ? inside[0] : outside[0];
                            const auto& others = oneInside ? outside : inside;
                            const int a = edgeVertex(ids[pivot], ids[others[0]], pos[pivot], pos[others[0]], val[pivot], val[others[0]]);
                            const int b = edgeVertex(ids[pivot], ids[others[1]], pos[pivot], pos[others[1]], val[pivot], val[others[1]]);
                            const int cidx = edgeVertex(ids[pivot], ids[others[2]], pos[pivot], pos[others[2]], val[pivot], val[others[2]]);
                            const Vec3 otherCentre = (pos[others[0]] + pos[others[1]] + pos[others[2]]) / 3.0f;
                            emit(a, b, cidx, oneInside ? otherCentre - pos[pivot] : pos[pivot] - otherCentre);
                        } else {
                            const int i0 = inside[0];
                            const int i1 = inside[1];
                            const int o0 = outside[0];
                            const int o1 = outside[1];
                            const int a = edgeVertex(ids[i0], ids[o0], pos[i0], pos[o0], val[i0], val[o0]);
                            const int b = edgeVertex(ids[i0], ids[o1], pos[i0], pos[o1], val[i0], val[o1]);
                            const int cidx = edgeVertex(ids[i1], ids[o0], pos[i1], pos[o0], val[i1], val[o0]);
                            const int d = edgeVertex(ids[i1], ids[o1], pos[i1], pos[o1], val[i1], val[o1]);
                            const Vec3 outwardHint = (pos[o0] + pos[o1]) * 0.5f - (pos[i0] + pos[i1]) * 0.5f;
                            emit(a, b, cidx, outwardHint);
                            emit(b, d, cidx, outwardHint);
                        }
                    }
                }
            }
        }

        keepExpectedComponents(vertices, triangles);
    }

private:
    const AsteroidField& field_;
    const Config& c_;

    static uint64_t edgeKey(int a, int b) {
        const uint32_t lo = static_cast<uint32_t>(std::min(a, b));
        const uint32_t hi = static_cast<uint32_t>(std::max(a, b));
        return (static_cast<uint64_t>(lo) << 32) | hi;
    }

    void keepExpectedComponents(std::vector<Vec3>& vertices, std::vector<Tri>& triangles) const {
        if (triangles.empty()) return;
        std::unordered_map<uint64_t, std::vector<int>> edgeOwners;
        edgeOwners.reserve(triangles.size() * 2);
        for (int i = 0; i < static_cast<int>(triangles.size()); ++i) {
            const Tri& t = triangles[i];
            edgeOwners[edgeKey(t.a, t.b)].push_back(i);
            edgeOwners[edgeKey(t.b, t.c)].push_back(i);
            edgeOwners[edgeKey(t.c, t.a)].push_back(i);
        }
        std::vector<std::vector<int>> adjacency(triangles.size());
        for (const auto& entry : edgeOwners) {
            const auto& owners = entry.second;
            for (size_t i = 0; i < owners.size(); ++i) {
                for (size_t j = i + 1; j < owners.size(); ++j) {
                    adjacency[owners[i]].push_back(owners[j]);
                    adjacency[owners[j]].push_back(owners[i]);
                }
            }
        }
        std::vector<char> visited(triangles.size(), 0);
        std::vector<std::vector<int>> components;
        for (int start = 0; start < static_cast<int>(triangles.size()); ++start) {
            if (visited[start]) continue;
            std::queue<int> queue;
            std::vector<int> component;
            visited[start] = 1;
            queue.push(start);
            while (!queue.empty()) {
                const int current = queue.front();
                queue.pop();
                component.push_back(current);
                for (int neighbour : adjacency[current]) {
                    if (!visited[neighbour]) {
                        visited[neighbour] = 1;
                        queue.push(neighbour);
                    }
                }
            }
            components.push_back(std::move(component));
        }
        const size_t expected = c_.topology == "detached" ? 2u : 1u;
        if (components.size() <= expected) return;
        std::sort(components.begin(), components.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });
        std::vector<char> keep(triangles.size(), 0);
        for (size_t i = 0; i < expected && i < components.size(); ++i) {
            for (int triIndex : components[i]) keep[triIndex] = 1;
        }
        std::vector<int> remap(vertices.size(), -1);
        std::vector<Vec3> newVertices;
        std::vector<Tri> newTriangles;
        newVertices.reserve(vertices.size());
        newTriangles.reserve(triangles.size());
        for (int i = 0; i < static_cast<int>(triangles.size()); ++i) {
            if (!keep[i]) continue;
            Tri t = triangles[i];
            int* indices[3] = {&t.a, &t.b, &t.c};
            for (int* index : indices) {
                if (remap[*index] < 0) {
                    remap[*index] = static_cast<int>(newVertices.size());
                    newVertices.push_back(vertices[*index]);
                }
                *index = remap[*index];
            }
            newTriangles.push_back(t);
        }
        vertices.swap(newVertices);
        triangles.swap(newTriangles);
    }
};

static MeshDiagnostics diagnoseMesh(const std::vector<Tri>& triangles) {
    MeshDiagnostics diagnostics;
    if (triangles.empty()) return diagnostics;
    auto makeEdgeKey = [](int a, int b) -> uint64_t {
        const uint32_t lo = static_cast<uint32_t>(std::min(a, b));
        const uint32_t hi = static_cast<uint32_t>(std::max(a, b));
        return (static_cast<uint64_t>(lo) << 32) | hi;
    };
    struct EdgeInfo { int count = 0; int orientation = 0; };
    std::unordered_map<uint64_t, EdgeInfo> edges;
    edges.reserve(triangles.size() * 2);
    std::unordered_map<int, std::vector<int>> vertexTriangles;
    vertexTriangles.reserve(triangles.size());
    for (int i = 0; i < static_cast<int>(triangles.size()); ++i) {
        const Tri& t = triangles[i];
        const int edgeA[3] = {t.a, t.b, t.c};
        const int edgeB[3] = {t.b, t.c, t.a};
        for (int e = 0; e < 3; ++e) {
            EdgeInfo& info = edges[makeEdgeKey(edgeA[e], edgeB[e])];
            ++info.count;
            info.orientation += edgeA[e] < edgeB[e] ? 1 : -1;
        }
        vertexTriangles[t.a].push_back(i);
        vertexTriangles[t.b].push_back(i);
        vertexTriangles[t.c].push_back(i);
    }
    for (const auto& entry : edges) {
        const EdgeInfo& info = entry.second;
        if (info.count == 1) ++diagnostics.boundaryEdges;
        else if (info.count != 2) ++diagnostics.nonManifoldEdges;
        else if (info.orientation != 0) ++diagnostics.windingErrors;
    }
    std::vector<char> visited(triangles.size(), 0);
    for (int start = 0; start < static_cast<int>(triangles.size()); ++start) {
        if (visited[start]) continue;
        ++diagnostics.components;
        std::queue<int> queue;
        queue.push(start);
        visited[start] = 1;
        while (!queue.empty()) {
            const int current = queue.front();
            queue.pop();
            const Tri& t = triangles[current];
            const int verts[3] = {t.a, t.b, t.c};
            for (int v : verts) {
                for (int next : vertexTriangles[v]) {
                    if (!visited[next]) {
                        visited[next] = 1;
                        queue.push(next);
                    }
                }
            }
        }
    }
    return diagnostics;
}


struct WasmMeshOutput {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    std::vector<float> surface;
    std::array<float, 6> centres{};
    std::array<float, 2> radii{};
    std::array<float, 18> bases{};
    std::array<int, 3> grid{};
    size_t boundaryEdges = 0;
    size_t nonManifoldEdges = 0;
    size_t windingErrors = 0;
    size_t components = 0;
    bool watertight = false;
    std::string name;
    std::string objectId;
    std::string error;
};

static WasmMeshOutput g_wasmOutput;

static void clearWasmOutput() {
    g_wasmOutput.vertices.clear();
    g_wasmOutput.indices.clear();
    g_wasmOutput.surface.clear();
    g_wasmOutput.centres.fill(0.0f);
    g_wasmOutput.radii.fill(0.0f);
    g_wasmOutput.bases.fill(0.0f);
    g_wasmOutput.grid.fill(0);
    g_wasmOutput.boundaryEdges = 0;
    g_wasmOutput.nonManifoldEdges = 0;
    g_wasmOutput.windingErrors = 0;
    g_wasmOutput.components = 0;
    g_wasmOutput.watertight = false;
    g_wasmOutput.name.clear();
    g_wasmOutput.objectId.clear();
    g_wasmOutput.error.clear();
}

extern "C" {

static int asteroidGenerateImpl(const char* query) {
    clearWasmOutput();
        const Config config = loadConfigFromQuery(query);
        g_wasmOutput.name = generatedAsteroidName(config.seed);
        g_wasmOutput.objectId = generatedObjectId(config.seed);

        const AsteroidField field(config);
        Mesher mesher(field, config);
        std::vector<Vec3> vertices;
        std::vector<Tri> triangles;
        int nx = 0;
        int ny = 0;
        int nz = 0;
        mesher.build(vertices, triangles, nx, ny, nz);
        const MeshDiagnostics diagnostics = diagnoseMesh(triangles);

        g_wasmOutput.vertices.resize(vertices.size() * 3);
        for (size_t i = 0; i < vertices.size(); ++i) {
            g_wasmOutput.vertices[i * 3 + 0] = vertices[i].x;
            g_wasmOutput.vertices[i * 3 + 1] = vertices[i].y;
            g_wasmOutput.vertices[i * 3 + 2] = vertices[i].z;
        }

        g_wasmOutput.indices.resize(triangles.size() * 3);
        for (size_t i = 0; i < triangles.size(); ++i) {
            g_wasmOutput.indices[i * 3 + 0] = static_cast<uint32_t>(triangles[i].a);
            g_wasmOutput.indices[i * 3 + 1] = static_cast<uint32_t>(triangles[i].b);
            g_wasmOutput.indices[i * 3 + 2] = static_cast<uint32_t>(triangles[i].c);
        }

        constexpr size_t surfaceStride = 10;
        g_wasmOutput.surface.resize(vertices.size() * surfaceStride);
        for (size_t i = 0; i < vertices.size(); ++i) {
            const SurfaceColourData data = field.surfaceColourData(vertices[i]);
            const size_t o = i * surfaceStride;
            g_wasmOutput.surface[o + 0] = static_cast<float>(data.lobe);
            g_wasmOutput.surface[o + 1] = data.lobeBlend;
            g_wasmOutput.surface[o + 2] = data.macro;
            g_wasmOutput.surface[o + 3] = data.roughness;
            g_wasmOutput.surface[o + 4] = data.craterBowl;
            g_wasmOutput.surface[o + 5] = data.craterRim;
            g_wasmOutput.surface[o + 6] = data.boulder;
            g_wasmOutput.surface[o + 7] = data.facet;
            g_wasmOutput.surface[o + 8] = data.groove;
            g_wasmOutput.surface[o + 9] = data.totalShift;
        }

        const Vec3 centre0 = field.centre(0);
        const Vec3 centre1 = field.centre(1);
        g_wasmOutput.centres = {centre0.x, centre0.y, centre0.z, centre1.x, centre1.y, centre1.z};
        g_wasmOutput.radii = {field.lobeRadius(0), field.lobeRadius(1)};

        const LobeBasis basis0 = field.lobeBasis(0);
        const LobeBasis basis1 = field.lobeBasis(1);
        const float basisValues[18] = {
            basis0.x.x, basis0.x.y, basis0.x.z,
            basis0.y.x, basis0.y.y, basis0.y.z,
            basis0.z.x, basis0.z.y, basis0.z.z,
            basis1.x.x, basis1.x.y, basis1.x.z,
            basis1.y.x, basis1.y.y, basis1.y.z,
            basis1.z.x, basis1.z.y, basis1.z.z
        };
        std::copy(std::begin(basisValues), std::end(basisValues), g_wasmOutput.bases.begin());

        g_wasmOutput.grid = {nx, ny, nz};
        g_wasmOutput.boundaryEdges = diagnostics.boundaryEdges;
        g_wasmOutput.nonManifoldEdges = diagnostics.nonManifoldEdges;
        g_wasmOutput.windingErrors = diagnostics.windingErrors;
        g_wasmOutput.components = diagnostics.components;
        g_wasmOutput.watertight = diagnostics.boundaryEdges == 0 && diagnostics.nonManifoldEdges == 0 && diagnostics.windingErrors == 0;
    return 0;
}

AST_EXPORT int asteroid_generate(const char* query) {
#if defined(ASTEROID_WASM)
    return asteroidGenerateImpl(query);
#else
    try {
        return asteroidGenerateImpl(query);
    } catch (const std::exception& e) {
        g_wasmOutput.error = e.what();
        return 1;
    } catch (...) {
        g_wasmOutput.error = "Unknown mesh generation error";
        return 2;
    }
#endif
}

AST_EXPORT int asteroid_mesh_version() { return 24; }
AST_EXPORT int asteroid_vertex_count() { return static_cast<int>(g_wasmOutput.vertices.size() / 3); }
AST_EXPORT int asteroid_triangle_count() { return static_cast<int>(g_wasmOutput.indices.size() / 3); }
AST_EXPORT const float* asteroid_vertices_ptr() { return g_wasmOutput.vertices.empty() ? nullptr : g_wasmOutput.vertices.data(); }
AST_EXPORT const uint32_t* asteroid_indices_ptr() { return g_wasmOutput.indices.empty() ? nullptr : g_wasmOutput.indices.data(); }
AST_EXPORT const float* asteroid_surface_ptr() { return g_wasmOutput.surface.empty() ? nullptr : g_wasmOutput.surface.data(); }
AST_EXPORT int asteroid_surface_stride() { return 10; }
AST_EXPORT const float* asteroid_centres_ptr() { return g_wasmOutput.centres.data(); }
AST_EXPORT const float* asteroid_radii_ptr() { return g_wasmOutput.radii.data(); }
AST_EXPORT const float* asteroid_bases_ptr() { return g_wasmOutput.bases.data(); }
AST_EXPORT int asteroid_grid_x() { return g_wasmOutput.grid[0]; }
AST_EXPORT int asteroid_grid_y() { return g_wasmOutput.grid[1]; }
AST_EXPORT int asteroid_grid_z() { return g_wasmOutput.grid[2]; }
AST_EXPORT int asteroid_boundary_edges() { return static_cast<int>(g_wasmOutput.boundaryEdges); }
AST_EXPORT int asteroid_non_manifold_edges() { return static_cast<int>(g_wasmOutput.nonManifoldEdges); }
AST_EXPORT int asteroid_winding_errors() { return static_cast<int>(g_wasmOutput.windingErrors); }
AST_EXPORT int asteroid_components() { return static_cast<int>(g_wasmOutput.components); }
AST_EXPORT int asteroid_watertight() { return g_wasmOutput.watertight ? 1 : 0; }
AST_EXPORT const char* asteroid_name() { return g_wasmOutput.name.c_str(); }
AST_EXPORT const char* asteroid_object_id() { return g_wasmOutput.objectId.c_str(); }
AST_EXPORT const char* asteroid_error() { return g_wasmOutput.error.c_str(); }

}

#if !defined(ASTEROID_WASM) && !defined(ASTEROID_NO_MAIN)
int main() {
    const Config config = loadConfig();
    const std::string asteroidName = generatedAsteroidName(config.seed);
    const std::string objectId = generatedObjectId(config.seed);
    const AsteroidField field(config);
    Mesher mesher(field, config);
    std::vector<Vec3> vertices;
    std::vector<Tri> triangles;
    int nx = 0;
    int ny = 0;
    int nz = 0;
    mesher.build(vertices, triangles, nx, ny, nz);
    const MeshDiagnostics diagnostics = diagnoseMesh(triangles);

    std::cout << "Content-Type: application/json\r\n";
    std::cout << "Cache-Control: no-store\r\n";
    std::cout << "Access-Control-Allow-Origin: *\r\n\r\n";
    std::cout << std::setprecision(7);
    std::cout << "{\n\"vertices\":[";
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i) std::cout << ',';
        std::cout << vertices[i].x << ',' << vertices[i].y << ',' << vertices[i].z;
    }
    std::cout << "],\n\"faces\":[";
    for (size_t i = 0; i < triangles.size(); ++i) {
        if (i) std::cout << ',';
        std::cout << triangles[i].a << ',' << triangles[i].b << ',' << triangles[i].c;
    }

    constexpr float surfaceScale = 4096.0f;
    std::cout << "],\n\"surfaceScale\":" << static_cast<int>(surfaceScale) << ',';
    std::cout << "\n\"surface\":[";
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (i) std::cout << ',';
        const SurfaceColourData data = field.surfaceColourData(vertices[i]);
        const auto quantise = [](float v) -> int {
            return static_cast<int>(std::lround(clampf(v, -1.0f, 1.0f) * surfaceScale));
        };
        std::cout << data.lobe << ','
                  << quantise(data.lobeBlend) << ','
                  << quantise(data.macro) << ','
                  << quantise(data.roughness) << ','
                  << quantise(data.craterBowl) << ','
                  << quantise(data.craterRim) << ','
                  << quantise(data.boulder) << ','
                  << quantise(data.facet) << ','
                  << quantise(data.groove) << ','
                  << quantise(data.totalShift);
    }
    std::cout << "],\n\"stats\":{";
    std::cout << "\"meshVersion\":23,";
    std::cout << "\"name\":\"" << asteroidName << "\",";
    std::cout << "\"objectId\":\"" << objectId << "\",";
    std::cout << "\"vertices\":" << vertices.size() << ',';
    std::cout << "\"triangles\":" << triangles.size() << ',';
    std::cout << "\"grid\":[" << nx << ',' << ny << ',' << nz << "],";
    const Vec3 centre0 = field.centre(0);
    const Vec3 centre1 = field.centre(1);
    const LobeBasis basis0 = field.lobeBasis(0);
    const LobeBasis basis1 = field.lobeBasis(1);
    std::cout << "\"centres\":[[" << centre0.x << ',' << centre0.y << ',' << centre0.z << "],[" << centre1.x << ',' << centre1.y << ',' << centre1.z << "]],";
    std::cout << "\"radii\":[" << field.lobeRadius(0) << ',' << field.lobeRadius(1) << "],";
    std::cout << "\"bases\":[["
              << basis0.x.x << ',' << basis0.x.y << ',' << basis0.x.z << ','
              << basis0.y.x << ',' << basis0.y.y << ',' << basis0.y.z << ','
              << basis0.z.x << ',' << basis0.z.y << ',' << basis0.z.z << "],["
              << basis1.x.x << ',' << basis1.x.y << ',' << basis1.x.z << ','
              << basis1.y.x << ',' << basis1.y.y << ',' << basis1.y.z << ','
              << basis1.z.x << ',' << basis1.z.y << ',' << basis1.z.z << "]],";
    std::cout << "\"boundaryEdges\":" << diagnostics.boundaryEdges << ',';
    std::cout << "\"nonManifoldEdges\":" << diagnostics.nonManifoldEdges << ',';
    std::cout << "\"windingErrors\":" << diagnostics.windingErrors << ',';
    std::cout << "\"components\":" << diagnostics.components << ',';
    std::cout << "\"watertight\":" << ((diagnostics.boundaryEdges == 0 && diagnostics.nonManifoldEdges == 0 && diagnostics.windingErrors == 0) ? "true" : "false");
    std::cout << "}\n}\n";
    return 0;
}
#endif
