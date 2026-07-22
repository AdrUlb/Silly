#pragma once
#include <source_location>
#include <string_view>

namespace Silly
{
	class [[nodiscard]] Error
	{
	public:
		explicit Error(const std::string_view message, const std::source_location location = std::source_location::current()) :
			_message(message), _location(location) {}

		[[nodiscard]] std::string_view GetMessage() const
		{
			return _message;
		}

		[[nodiscard]] std::source_location GetLocation() const
		{
			return _location;
		}

	private:
		const std::string_view _message;
		const std::source_location _location;
	};
}
