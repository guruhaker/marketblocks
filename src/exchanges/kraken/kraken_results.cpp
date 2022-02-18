#include "kraken_results.h"
#include "common/file/json.h"
#include "common/utils/stringutils.h"
#include "logging/logger.h"

namespace
{
	std::string get_error(cb::json_document& json)
	{
		std::vector<std::string> messages = json.get<std::vector<std::string>>("error");
		
		return messages.empty()
			? ""
			: messages[0];
	}
}

namespace cb::internal
{
	result<exchange_status> read_system_status(const std::string& jsonResult)
	{
		json_document jsonDocument{ parse_json(jsonResult) };

		std::string error = get_error(jsonDocument);
		if (!error.empty())
		{
			return result<exchange_status>::fail(std::move(error));
		}

		json_element resultElement{ jsonDocument.element("result") };

		std::string status_string{ resultElement.get<std::string>("status") };
		exchange_status status;

		if (status_string == "online")
		{
			status = exchange_status::ONLINE;
		}
		else if (status_string == "cancel_only")
		{
			status = exchange_status::CANCEL_ONLY;
		}
		else if (status_string == "post_only")
		{
			status = exchange_status::POST_ONLY;
		}
		else
		{
			status = exchange_status::MAINTENANCE;
		}

		return result<exchange_status>::success(std::move(status));
	}

	result<std::vector<tradable_pair>> read_tradable_pairs(const std::string& jsonResult)
	{
		json_document jsonDocument{ parse_json(jsonResult) };

		std::string error = get_error(jsonDocument);
		if (!error.empty())
		{
			return result<std::vector<tradable_pair>>::fail(std::move(error));
		}

		json_element resultElement{ jsonDocument.element("result") };

		std::vector<tradable_pair> pairs;
		pairs.reserve(resultElement.size());

		for (auto it = resultElement.begin(); it != resultElement.end(); ++it)
		{
			std::string name{ it.key() };
			std::string wsName{ it.value().get<std::string>("wsname") };
			std::vector<std::string> assetSymbols{ split(wsName, '/') };
			pairs.emplace_back(name, asset_symbol{ assetSymbols[0] }, asset_symbol{ assetSymbols[1] });
		}

		return result<std::vector<tradable_pair>>::success(std::move(pairs));
	}

	result<order_book_state> read_order_book(const std::string& jsonResult)
	{
		json_document jsonDocument{ parse_json(jsonResult) };

		std::string error = get_error(jsonDocument);
		if (!error.empty())
		{
			return result<order_book_state>::fail(std::move(error));
		}

		json_element resultElement{ jsonDocument
			.element("result")
			.element(0) };

		json_element asks{ resultElement.element("asks") };
		json_element bids{ resultElement.element("bids") };

		int depth = asks.size();
		std::vector<order_book_level> levels;
		levels.reserve(depth);

		for (int i = 0; i < depth; i++)
		{
			json_element asks_i{ asks.element(i) };
			order_book_entry askEntry
			{
				order_book_side::ASK,
				std::stod(asks_i.element(0).get<std::string>()),
				std::stod(asks_i.element(1).get<std::string>()),
				asks_i.element(2).get<double>()
			};

			json_element bids_i{ bids.element(i) };
			order_book_entry bidEntry
			{
				order_book_side::BID,
				std::stod(bids_i.element(0).get<std::string>()),
				std::stod(bids_i.element(1).get<std::string>()),
				bids_i.element(2).get<double>()
			};

			levels.emplace_back(std::move(askEntry), std::move(bidEntry));
		}

		return result<order_book_state>::success(std::move(levels));
	}

	result<std::unordered_map<asset_symbol, double>> read_balances(const std::string& jsonResult)
	{
		json_document jsonDocument{ parse_json(jsonResult) };

		std::string error = get_error(jsonDocument);
		if (!error.empty())
		{
			return result<std::unordered_map<asset_symbol, double>>::fail(std::move(error));
		}

		return result<std::unordered_map<asset_symbol, double>>::success(std::unordered_map<asset_symbol, double>());
	}
}