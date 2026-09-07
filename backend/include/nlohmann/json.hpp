#pragma once

#ifndef NLOHMANN_JSON_HPP
#define NLOHMANN_JSON_HPP

#include <initializer_list>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace nlohmann {

class json {
public:
    using object_t = std::map<std::string, json>;
    using array_t = std::vector<json>;

    enum class Type {
        null_value,
        boolean,
        integer,
        number,
        string,
        array,
        object
    };

    json()
        : type_(Type::null_value), boolean_value_(false), integer_value_(0), number_value_(0.0) {}

    json(std::nullptr_t)
        : json() {}

    json(bool value)
        : type_(Type::boolean), boolean_value_(value), integer_value_(0), number_value_(0.0) {}

    json(int value)
        : type_(Type::integer), boolean_value_(false), integer_value_(value), number_value_(0.0) {}

    json(long value)
        : type_(Type::integer), boolean_value_(false), integer_value_(value), number_value_(0.0) {}

    json(long long value)
        : type_(Type::integer), boolean_value_(false), integer_value_(value), number_value_(0.0) {}

    json(unsigned int value)
        : type_(Type::integer), boolean_value_(false), integer_value_(value), number_value_(0.0) {}

    json(unsigned long value)
        : type_(Type::integer), boolean_value_(false), integer_value_(value), number_value_(0.0) {}

    json(unsigned long long value)
        : type_(Type::integer), boolean_value_(false), integer_value_(static_cast<long long>(value)), number_value_(0.0) {}

    json(double value)
        : type_(Type::number), boolean_value_(false), integer_value_(0), number_value_(value) {}

    json(const char* value)
        : type_(Type::string), boolean_value_(false), integer_value_(0), number_value_(0.0), string_value_(value ? value : "") {}

    json(const std::string& value)
        : type_(Type::string), boolean_value_(false), integer_value_(0), number_value_(0.0), string_value_(value) {}

    json(const array_t& value)
        : type_(Type::array), boolean_value_(false), integer_value_(0), number_value_(0.0), array_value_(value) {}

    json(const object_t& value)
        : type_(Type::object), boolean_value_(false), integer_value_(0), number_value_(0.0), object_value_(value) {}

    json(const std::vector<int>& values)
        : type_(Type::array), boolean_value_(false), integer_value_(0), number_value_(0.0) {
        for (int value : values) {
            array_value_.push_back(json(value));
        }
    }

    json(const std::vector<std::string>& values)
        : type_(Type::array), boolean_value_(false), integer_value_(0), number_value_(0.0) {
        for (const auto& value : values) {
            array_value_.push_back(json(value));
        }
    }

    json(std::initializer_list<std::pair<const std::string, json>> init)
        : type_(Type::object), boolean_value_(false), integer_value_(0), number_value_(0.0) {
        for (const auto& entry : init) {
            object_value_[entry.first] = entry.second;
        }
    }

    static json array() {
        return json(array_t{});
    }

    bool contains(const std::string& key) const {
        return type_ == Type::object && object_value_.count(key) > 0;
    }

    std::size_t size() const {
        if (type_ == Type::array) {
            return array_value_.size();
        }
        if (type_ == Type::object) {
            return object_value_.size();
        }
        return 0;
    }

    bool empty() const {
        return size() == 0;
    }

    const json& operator[](const std::string& key) const {
        static const json null_json;
        if (type_ != Type::object) {
            return null_json;
        }

        const auto it = object_value_.find(key);
        return it != object_value_.end() ? it->second : null_json;
    }

    json& operator[](const std::string& key) {
        ensure_object();
        return object_value_[key];
    }

    const json& operator[](std::size_t index) const {
        static const json null_json;
        if (type_ != Type::array || index >= array_value_.size()) {
            return null_json;
        }

        return array_value_[index];
    }

    json& operator[](std::size_t index) {
        ensure_array();
        if (index >= array_value_.size()) {
            array_value_.resize(index + 1);
        }
        return array_value_[index];
    }

    long long as_int64(long long fallback = 0) const {
        if (type_ == Type::integer) {
            return integer_value_;
        }
        if (type_ == Type::number) {
            return static_cast<long long>(number_value_);
        }
        return fallback;
    }

    double as_double(double fallback = 0.0) const {
        if (type_ == Type::number) {
            return number_value_;
        }
        if (type_ == Type::integer) {
            return static_cast<double>(integer_value_);
        }
        return fallback;
    }

    bool as_bool(bool fallback = false) const {
        return type_ == Type::boolean ? boolean_value_ : fallback;
    }

    std::string as_string(const std::string& fallback = "") const {
        return type_ == Type::string ? string_value_ : fallback;
    }

    void push_back(const json& value) {
        ensure_array();
        array_value_.push_back(value);
    }

    std::string dump(int indent = -1) const {
        return dump_impl(indent, 0);
    }

private:
    Type type_;
    bool boolean_value_;
    long long integer_value_;
    double number_value_;
    std::string string_value_;
    array_t array_value_;
    object_t object_value_;

    void ensure_array() {
        if (type_ != Type::array) {
            type_ = Type::array;
            array_value_.clear();
            object_value_.clear();
            string_value_.clear();
            boolean_value_ = false;
            integer_value_ = 0;
            number_value_ = 0.0;
        }
    }

    void ensure_object() {
        if (type_ != Type::object) {
            type_ = Type::object;
            array_value_.clear();
            object_value_.clear();
            string_value_.clear();
            boolean_value_ = false;
            integer_value_ = 0;
            number_value_ = 0.0;
        }
    }

    static std::string escape(const std::string& value) {
        std::string escaped;
        for (char c : value) {
            switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
            }
        }
        return escaped;
    }

    static std::string indent_string(int indent, int level) {
        return std::string(indent * level, ' ');
    }

    std::string dump_impl(int indent, int level) const {
        switch (type_) {
        case Type::null_value:
            return "null";
        case Type::boolean:
            return boolean_value_ ? "true" : "false";
        case Type::integer:
            return std::to_string(integer_value_);
        case Type::number: {
            std::ostringstream oss;
            oss << number_value_;
            return oss.str();
        }
        case Type::string:
            return "\"" + escape(string_value_) + "\"";
        case Type::array:
            return dump_array(indent, level);
        case Type::object:
            return dump_object(indent, level);
        }

        return "null";
    }

    std::string dump_array(int indent, int level) const {
        if (array_value_.empty()) {
            return "[]";
        }

        std::ostringstream oss;
        oss << "[";

        if (indent >= 0) {
            oss << "\n";
            for (std::size_t i = 0; i < array_value_.size(); ++i) {
                oss << indent_string(indent, level + 1)
                    << array_value_[i].dump_impl(indent, level + 1);
                if (i + 1 < array_value_.size()) {
                    oss << ",";
                }
                oss << "\n";
            }
            oss << indent_string(indent, level) << "]";
        } else {
            for (std::size_t i = 0; i < array_value_.size(); ++i) {
                if (i > 0) {
                    oss << ",";
                }
                oss << array_value_[i].dump_impl(indent, level);
            }
            oss << "]";
        }

        return oss.str();
    }

    std::string dump_object(int indent, int level) const {
        if (object_value_.empty()) {
            return "{}";
        }

        std::ostringstream oss;
        oss << "{";

        if (indent >= 0) {
            oss << "\n";
            std::size_t index = 0;
            for (const auto& entry : object_value_) {
                oss << indent_string(indent, level + 1)
                    << "\"" << escape(entry.first) << "\": "
                    << entry.second.dump_impl(indent, level + 1);
                if (index + 1 < object_value_.size()) {
                    oss << ",";
                }
                oss << "\n";
                ++index;
            }
            oss << indent_string(indent, level) << "}";
        } else {
            std::size_t index = 0;
            for (const auto& entry : object_value_) {
                if (index > 0) {
                    oss << ",";
                }
                oss << "\"" << escape(entry.first) << "\":" << entry.second.dump_impl(indent, level);
                ++index;
            }
            oss << "}";
        }

        return oss.str();
    }
};

}  // namespace nlohmann

#endif
