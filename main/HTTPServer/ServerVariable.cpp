#include "ServerVariable.h"

ServerVariable::ServerVariable(std::shared_ptr<std::string> storage,
                               std::function<bool(std::string_view)> validator,
                               std::string_view paramName)
        : Storage(std::move(storage)),
          Validator(std::move(validator)),
          Getter(nullptr),
          Setter(nullptr),
          ParamName(paramName) {
    if (!Storage) {
        throw std::invalid_argument("Storage cannot be nullptr");
    }
    if (!Validator) {
        Validator = [](std::string_view) { return true; };
    }
}

ServerVariable::ServerVariable(std::function<std::string()> getter,
                               std::function<bool(std::string_view)> setter,
                               std::string_view paramName)
        : Storage(nullptr),
          Validator(nullptr),
          Getter(std::move(getter)),
          Setter(std::move(setter)),
          ParamName(paramName) {
    if (!Getter) {
        throw std::invalid_argument("Getter cannot be nullptr");
    }
    if (!Setter) {
        throw std::invalid_argument("Setter cannot be nullptr");
    }
}

bool ServerVariable::Set(std::string_view value) {
    if (Storage) {
        if (Validator && !Validator(value)) {
            return false;
        }
        *Storage = value;
        return true;
    }
    else {
        return Setter(value);
    }
}

std::string ServerVariable::Get() const {
    if (Storage) { return *Storage; }
    else { return Getter(); }
}
