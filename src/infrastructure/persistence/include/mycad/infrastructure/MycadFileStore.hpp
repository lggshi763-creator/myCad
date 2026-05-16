#pragma once

#include <string>
#include <string_view>

/// @file .mycad ZIP container reader / writer (Sprint 0.5).

namespace mycad::infrastructure {

/// @brief Reads and writes the .mycad file format.
///
/// A .mycad file is a ZIP archive containing:
/// @code
///   manifest.json   — format version, app version, creation timestamp
///   events.db       — SQLite database from SqliteEventStore
/// @endcode
///
/// This class contains only static utility methods; it holds no state.
class MycadFileStore {
public:
    MycadFileStore() = delete;

    /// @brief Packs a SqliteEventStore database into a .mycad ZIP file.
    ///
    /// @param dbPath      Absolute path to the source events.db file.
    /// @param outputPath  Destination path for the .mycad file (created or overwritten).
    /// @throws std::runtime_error on any I/O or ZIP error.
    static void save(std::string_view dbPath, std::string_view outputPath);

    /// @brief Extracts events.db from a .mycad ZIP into extractDir.
    ///
    /// @param mycadPath  Absolute path to the source .mycad file.
    /// @param extractDir Directory where events.db will be written (must exist).
    /// @return           Absolute path to the extracted events.db file.
    /// @throws std::runtime_error if the file cannot be opened or events.db is missing.
    [[nodiscard]] static std::string load(std::string_view mycadPath, std::string_view extractDir);
};

}  // namespace mycad::infrastructure
