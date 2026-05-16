#pragma once

#include <string>

/// @file Ambient context forwarded to every command handler.

namespace mycad::application {

/// @brief Ambient metadata forwarded to every ICommandHandler::handle call.
///
/// userId and operationId are opaque strings in Sprint 0.4.  They will
/// be replaced by typed UserId / OperationId value objects when the
/// identity bounded-context lands.
struct CommandContext {
    std::string userId;       ///< Opaque identifier of the acting user.
    std::string operationId;  ///< Opaque per-operation correlation id.
    std::string source;       ///< Origin tag: "ui", "test", "plugin:xyz".
};

}  // namespace mycad::application
