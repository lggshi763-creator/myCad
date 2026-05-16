#include <mycad/infrastructure/MycadFileStore.hpp>
#include <zip.h>

#include <array>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mycad::infrastructure {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// @brief Returns the current UTC time as an ISO-8601 string, e.g. "2026-06-05T10:00:00Z".
std::string utcNowIso8601() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::array<char, 32> buf{};
    std::strftime(buf.data(), buf.size(), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buf.data());
}

/// @brief Builds the manifest.json content string.
std::string buildManifest() {
    return "{\n"
           "  \"formatVersion\": 1,\n"
           "  \"appVersion\": \"0.0.1\",\n"
           "  \"createdAt\": \"" +
           utcNowIso8601() +
           "\",\n"
           "  \"thumbnail\": null\n"
           "}\n";
}

/// @brief Throws std::runtime_error with a libzip error message.
[[noreturn]] void throwZipError(zip_t* archive, const std::string& context) {
    const std::string msg = archive ? zip_strerror(archive) : "zip error";
    throw std::runtime_error(context + ": " + msg);
}

/// @brief RAII wrapper for zip_t that always calls zip_discard on failure.
struct ZipGuard {
    zip_t* z{nullptr};
    explicit ZipGuard(zip_t* z) : z(z) {}
    ~ZipGuard() {
        if (z) {
            zip_discard(z);
        }
    }
    /// @brief Releases ownership (call before zip_close on success).
    zip_t* release() {
        zip_t* tmp = z;
        z = nullptr;
        return tmp;
    }
};

}  // namespace

// ---------------------------------------------------------------------------
// save
// ---------------------------------------------------------------------------

void MycadFileStore::save(std::string_view dbPath, std::string_view outputPath) {
    const std::string outStr{outputPath};
    const std::string dbStr{dbPath};

    // Open (or create / truncate) the ZIP archive.
    int err = 0;
    ZipGuard guard{zip_open(outStr.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err)};
    if (!guard.z) {
        zip_error_t ze;
        zip_error_init_with_code(&ze, err);
        const std::string msg = zip_error_strerror(&ze);
        zip_error_fini(&ze);
        throw std::runtime_error("MycadFileStore::save: cannot create '" + outStr + "': " + msg);
    }

    // --- Add manifest.json from in-memory buffer ---
    const std::string manifest = buildManifest();
    // zip_source_buffer does NOT take ownership when freep=0; keep `manifest` alive until close.
    zip_source_t* manifestSrc = zip_source_buffer(guard.z, manifest.data(), manifest.size(), 0);
    if (!manifestSrc) {
        throwZipError(guard.z, "MycadFileStore::save: zip_source_buffer(manifest)");
    }
    if (zip_file_add(guard.z, "manifest.json", manifestSrc, ZIP_FL_OVERWRITE) < 0) {
        zip_source_free(manifestSrc);
        throwZipError(guard.z, "MycadFileStore::save: zip_file_add(manifest.json)");
    }

    // --- Add events.db from file ---
    zip_source_t* dbSrc = zip_source_file(guard.z, dbStr.c_str(), 0, -1);
    if (!dbSrc) {
        throwZipError(guard.z, "MycadFileStore::save: zip_source_file(events.db)");
    }
    if (zip_file_add(guard.z, "events.db", dbSrc, ZIP_FL_OVERWRITE) < 0) {
        zip_source_free(dbSrc);
        throwZipError(guard.z, "MycadFileStore::save: zip_file_add(events.db)");
    }

    // Commit: zip_close writes the archive and frees internal state.
    if (zip_close(guard.release()) != 0) {
        // guard.release() transferred ownership; re-open to get the error message.
        throw std::runtime_error("MycadFileStore::save: zip_close failed for '" + outStr + "'");
    }
}

// ---------------------------------------------------------------------------
// load
// ---------------------------------------------------------------------------

std::string MycadFileStore::load(std::string_view mycadPath, std::string_view extractDir) {
    const std::string srcStr{mycadPath};

    int err = 0;
    ZipGuard guard{zip_open(srcStr.c_str(), ZIP_RDONLY, &err)};
    if (!guard.z) {
        zip_error_t ze;
        zip_error_init_with_code(&ze, err);
        const std::string msg = zip_error_strerror(&ze);
        zip_error_fini(&ze);
        throw std::runtime_error("MycadFileStore::load: cannot open '" + srcStr + "': " + msg);
    }

    // Locate events.db inside the archive.
    zip_stat_t sb;
    if (zip_stat(guard.z, "events.db", 0, &sb) != 0) {
        throw std::runtime_error("MycadFileStore::load: 'events.db' not found in '" + srcStr + "'");
    }

    // Open and read the entry.
    zip_file_t* zf = zip_fopen(guard.z, "events.db", 0);
    if (!zf) {
        throwZipError(guard.z, "MycadFileStore::load: zip_fopen(events.db)");
    }

    std::vector<char> buf(static_cast<std::size_t>(sb.size));
    const zip_int64_t nread = zip_fread(zf, buf.data(), buf.size());
    zip_fclose(zf);

    if (nread < 0 || static_cast<zip_uint64_t>(nread) != sb.size) {
        throwZipError(guard.z, "MycadFileStore::load: zip_fread(events.db) incomplete");
    }

    // Write to extractDir/events.db.
    const std::string outPath = std::string{extractDir} + "/events.db";
    std::ofstream ofs{outPath, std::ios::binary | std::ios::trunc};
    if (!ofs) {
        throw std::runtime_error("MycadFileStore::load: cannot write '" + outPath + "'");
    }
    ofs.write(buf.data(), static_cast<std::streamsize>(buf.size()));
    if (!ofs) {
        throw std::runtime_error("MycadFileStore::load: write failed for '" + outPath + "'");
    }

    return outPath;
}

}  // namespace mycad::infrastructure
