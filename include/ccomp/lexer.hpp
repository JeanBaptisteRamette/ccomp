#ifndef CCOMP_LEXER_HPP
#define CCOMP_LEXER_HPP


#include <string_view>
#include <exception>
#include <variant>
#include <memory>
#include <vector>
#include <format>

#include <ccomp/source_location.hpp>
#include <ccomp/stream.hpp>
#include <ccomp/error.hpp>


namespace ccomp
{
    enum class token_type
    {
        eof,

		numerical,           // [0-9-a-f-A-F]
        byte_ascii,          // 'A' (quotes included)
        keyword_define,      // define x ...
		keyword_raw,         // raw(...)
		keyword_proc_start,  // proc ...
		keyword_proc_end,    // endp
        identifier,          // constants defined with the "define" keywords and label names
        instruction,         // call, ret, jmp, cls...
		register_name,       // special and general purpose registers
		bracket_open,
		bracket_close,
		parenthesis_open,
		parenthesis_close,
		colon,
		dot_label,
		comma
    };


	struct token
    {
        token_type type;
		source_location source_location;

		std::variant<uint16_t, std::string> data;
    };


    class lexer final
    {
    public:
        [[nodiscard]]
        static std::unique_ptr<lexer> from_file(std::string_view path, error_code& ec);

#ifdef UNIT_TESTS_ON
        [[nodiscard]]
        static std::unique_ptr<lexer> from_buffer(std::string_view buff);
#endif

        explicit lexer(ccomp::stream&& istream);
        ~lexer() = default;

        lexer(const lexer&)            = delete;
        lexer(lexer&&)                 = delete;
        lexer& operator=(const lexer&) = delete;
        lexer& operator=(lexer&&)      = delete;

		[[nodiscard]] std::vector<token> enumerate_tokens();

    private:
		[[nodiscard]] token next_token();

        [[nodiscard]]
        char peek_chr() const;
        char next_chr();

        void skip_comment();
        void skip_wspaces();

		[[nodiscard]] token make_token(token_type type, std::string lexeme = {}) const;
		[[nodiscard]] token make_numerical_token(uint16_t numerical_value) const;

		[[nodiscard]] uint16_t    read_numeric_lexeme();
        [[nodiscard]] std::string read_alpha_lexeme();

    private:
        ccomp::stream istream;
        source_location cursor;
    };


    namespace lexer_exception
    {
		struct lexer_error : std::runtime_error
		{
			explicit lexer_error(std::string_view message)
				: std::runtime_error(message.data())
			{}

			template<typename ...Args>
			explicit lexer_error(std::string_view fmt_message, Args&&... args)
				: std::runtime_error(std::vformat(fmt_message, std::make_format_args(args...)))
			{}
		};

        struct invalid_digit_for_base :lexer_error
        {
            invalid_digit_for_base(char digit, int base, const source_location& source_loc)
                : lexer_error(
						"Invalid digit \"{}\" for numeric base {} at {}.",
						digit,
						base,
						ccomp::to_string(source_loc))
            {}
        };

		struct numeric_constant_too_large : lexer_error
		{
			numeric_constant_too_large(std::string numeric_lexeme, const source_location& source_loc)
				: lexer_error(
						"Numeric constant \"{}\" at {} is too large for a 16-bit value.",
						numeric_lexeme,
						ccomp::to_string(source_loc))
			{}
		};

        struct undefined_character_token : lexer_error
        {
            explicit undefined_character_token(char c, const source_location& source_loc)
                : lexer_error(
						"Character \"{}\" cannot match any token at {}.",
						c,
						ccomp::to_string(source_loc))
            {}
        };
    }

	[[nodiscard]]
	constexpr std::string_view to_string(token_type type)
	{
		switch (type)
		{
			case token_type::eof:
				return "eof";
			case token_type::numerical:
				return "numerical";
			case token_type::byte_ascii:
				return "ascii";
			case token_type::keyword_define:
				return "define";
			case token_type::keyword_raw:
				return "raw";
			case token_type::identifier:
				return "identifier";
			case token_type::instruction:
				return "instruction";
			case token_type::register_name:
				return "register name";
			case token_type::bracket_open:
				return "open bracket";
			case token_type::bracket_close:
				return "close bracket";
			case token_type::parenthesis_open:
				return "open parenthesis";
			case token_type::parenthesis_close:
				return "close parenthesis";
			case token_type::colon:
				return "colon";
			case token_type::comma:
				return "comma";
			case token_type::dot_label:
				return "dot";

			default:
				return "undefined";
		}
	}

	[[nodiscard]]
	inline std::string to_string(std::initializer_list<token_type> types)
	{
		std::string joined;

		for (const auto type : types)
		{
			if (!joined.empty())
				joined += ", ";

			joined += ccomp::to_string(type);
		}

		return '(' + joined + ')';
	}

	[[nodiscard]]
	inline std::string to_string(const token& token)
	{
		if (std::holds_alternative<uint16_t>(token.data))
			return std::to_string(std::get<uint16_t>(token.data));

		return std::get<std::string>(token.data);
	}
}


#endif //CCOMP_LEXER_HPP
