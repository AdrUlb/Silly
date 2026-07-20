#include "Silly/StringView.hpp"

#include "Silly/String.hpp"

namespace Silly
{
	[[nodiscard]] Result<void, Error> StringView::AppendToStringImpl(String& str, const StringView& view)
	{
		return str.Append(view);
	}
}
