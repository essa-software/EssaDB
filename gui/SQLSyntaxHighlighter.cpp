#include "SQLSyntaxHighlighter.hpp"

#include <db/sql/Lexer.hpp>
#include <sstream>

namespace EssaDB {

enum class HighlightStyle {
    Keyword,
    String,
    Number,
    Constant,
    Comment
};

std::vector<GUI::TextStyle> SQLSyntaxHighlighter::styles() const {
    return {
        GUI::TextStyle { .color = Util::Color { 0x58d36fff } }, // Keyword
        GUI::TextStyle { .color = Util::Color { 0xbc894fff } }, // String
        GUI::TextStyle { .color = Util::Color { 0xdca6eaff } }, // Number
        GUI::TextStyle { .color = Util::Color { 0x26b1f2ff } }, // Constant
        GUI::TextStyle { .color = Util::Color { 0x679335ff } }, // Comment
    };
}

std::vector<GUI::StyledTextSpan> SQLSyntaxHighlighter::spans(Util::UString const& input) const {
    // TODO: Find a way to stream a UString. Maybe utf8 encode on the fly?
    auto encoded_input = input.encode();
    std::istringstream iss { encoded_input };
    Db::Sql::Lexer lexer { iss };
    auto tokens = lexer.lex();

    std::vector<GUI::StyledTextSpan> spans;

    auto create_span = [](Db::Sql::Token const& token, HighlightStyle style) {
        return GUI::StyledTextSpan { .span_start = static_cast<size_t>(token.start), .span_size = static_cast<size_t>(token.end - token.start), .style_index = static_cast<size_t>(style) };
    };

    for (auto const& token : tokens) {
        if (token.is_keyword()) {
            spans.push_back(create_span(token, HighlightStyle::Keyword));
        }
        else if (token.type == Db::Sql::Token::Type::String) {
            spans.push_back(create_span(token, HighlightStyle::String));
        }
        else if (token.type == Db::Sql::Token::Type::Int || token.type == Db::Sql::Token::Type::Float || token.type == Db::Sql::Token::Type::Date) {
            spans.push_back(create_span(token, HighlightStyle::Number));
        }
        else if (token.type == Db::Sql::Token::Type::Bool) {
            spans.push_back(create_span(token, HighlightStyle::Constant));
        }
    }

    return spans;
}

}
