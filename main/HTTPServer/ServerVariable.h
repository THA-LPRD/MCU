#ifndef LPRD_MCU_SERVERVARIABLE_H
#define LPRD_MCU_SERVERVARIABLE_H

#include <string>
#include <string_view>
#include <functional>
#include <memory>

class ServerVariable {
public:
    ServerVariable(std::shared_ptr<std::string> storage,
                   std::function<bool(std::string_view)> validator,
                   std::string_view paramName);
    ServerVariable(std::function<std::string()> getter,
                   std::function<bool(std::string_view)> setter,
                   std::string_view paramName);
    bool Set(std::string_view value);
    std::string Get() const;
    inline std::string_view GetName() const { return ParamName; }
    ServerVariable(ServerVariable &&) = delete;
    ServerVariable &operator=(ServerVariable &&) = delete;
    ServerVariable(const ServerVariable &) = delete;
    ServerVariable &operator=(const ServerVariable &) = delete;
private:
    std::shared_ptr<std::string> Storage;
    std::function<bool(std::string_view)> Validator;

    std::function<std::string()> Getter;
    std::function<bool(std::string_view)> Setter;

    std::string ParamName;
};

#endif //LPRD_MCU_SERVERVARIABLE_H
