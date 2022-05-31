#include "coinbase.h"
#include "coinbase_results.h"
#include "common/utils/timeutils.h"
#include "common/security/hash.h"
#include "common/security/encoding.h"

namespace
{
	using namespace mb;

	constexpr std::string_view to_string(order_type orderType)
	{
		constexpr std::string_view LIMIT = "limit";
		constexpr std::string_view MARKET = "market";
		constexpr std::string_view STOP_LOSS = "loss";
		constexpr std::string_view STOP_ENTRY = "entry";

		switch (orderType)
		{
		case order_type::LIMIT:
			return LIMIT;
		case order_type::MARKET:
			return MARKET;
		case order_type::STOP_LOSS:
			return STOP_LOSS;
		case order_type::TAKE_PROFIT:
			return STOP_ENTRY;
		default:
			throw mb_exception{ "Unknown order type" };
		}
	}

	constexpr bool is_stop_order(order_type orderType)
	{
		return orderType == order_type::STOP_LOSS || orderType == order_type::TAKE_PROFIT;
	}
}

namespace mb
{
	coinbase_api::coinbase_api(
		coinbase_config config,
		std::unique_ptr<http_service> httpService,
		std::unique_ptr<websocket_stream> websocketStream,
		bool enableTesting)
		:
		_constants{ enableTesting },
		_userAgentId{ get_timestamp() },
		_apiKey{ config.api_key() },
		_decodedApiSecret{ b64_decode(config.api_secret()) },
		_apiPassphrase{ config.api_passphrase() },
		_httpService{ std::move(httpService) }, 
		_websocketStream{ std::move(websocketStream) }
	{}

	std::string coinbase_api::get_timestamp() const
	{
		return std::to_string(time_since_epoch<std::chrono::seconds>());
	}

	std::string coinbase_api::compute_access_sign(std::string_view timestamp, http_verb httpVerb, std::string_view path, std::string_view query, std::string_view body) const
	{
		std::string message;
		message.append(timestamp);
		message.append(to_string(httpVerb));
		message.append(path);
		append_query(message, query);
		message.append(body);
		
		auto hmac = hmac_sha256(message, _decodedApiSecret);
		auto encoded = b64_encode(hmac);
		return encoded;
	}

	exchange_status coinbase_api::get_status() const
	{
		return exchange_status::ONLINE;
	}

	std::vector<tradable_pair> coinbase_api::get_tradable_pairs() const
	{
		std::string path{ build_url_path(std::array<std::string_view, 1>
		{
			_constants.methods.PRODUCTS
		}) };

		return send_public_request<std::vector<tradable_pair>>(path, coinbase::read_tradable_pairs);
	}

	ohlcv_data coinbase_api::get_24h_stats(const tradable_pair& tradablePair) const
	{
		std::string path{ build_url_path(std::array<std::string_view, 3>
		{
			_constants.methods.PRODUCTS,
			internal::to_exchange_id(tradablePair),
			_constants.methods.STATS
		})};

		return send_public_request<ohlcv_data>(path, coinbase::read_24h_stats);
	}

	double coinbase_api::get_price(const tradable_pair& tradablePair) const
	{
		std::string path{ build_url_path(std::array<std::string_view, 3>
		{
			_constants.methods.PRODUCTS,
			internal::to_exchange_id(tradablePair),
			_constants.methods.TICKER
		}) };

		return send_public_request<double>(path, coinbase::read_price);
	}

	order_book_state coinbase_api::get_order_book(const tradable_pair& tradablePair, int depth) const
	{
		std::string path{ build_url_path(std::array<std::string_view, 3>
		{
			_constants.methods.PRODUCTS,
			internal::to_exchange_id(tradablePair),
			_constants.methods.BOOK
		})};

		int level = depth == 1 
			? 1 
			: 2;

		std::string query{ url_query_builder{}
			.add_parameter(_constants.queries.LEVEL, std::to_string(level))
			.to_string() };

		return send_public_request<order_book_state>(
			path, 
			[depth](std::string_view jsonResult) { return coinbase::read_order_book(jsonResult, depth); },
			query);
	}

	double coinbase_api::get_fee(const tradable_pair& tradablePair) const
	{
		std::string path{ build_url_path(std::array<std::string_view, 1>
		{
			_constants.methods.FEES
		}) };

		return send_private_request<double>(http_verb::GET, path, coinbase::read_fee);
	}

	unordered_string_map<double> coinbase_api::get_balances() const
	{
		std::string path{ build_url_path(std::array<std::string_view, 1>
		{
			_constants.methods.COINBASE_ACCOUNTS
		}) };

		return send_private_request<unordered_string_map<double>>(http_verb::GET, path, coinbase::read_balances);
	}

	std::vector<order_description> coinbase_api::get_open_orders() const
	{
		std::string path{ build_url_path(std::array<std::string_view, 1>
		{
			_constants.methods.ORDERS
		}) };

		return send_private_request<std::vector<order_description>>(http_verb::GET, path, coinbase::read_orders);
	}

	std::vector<order_description> coinbase_api::get_closed_orders() const
	{
		static constexpr std::string_view DONE = "done";

		std::string path{ build_url_path(std::array<std::string_view, 1>
		{
			_constants.methods.ORDERS
		}) };

		std::string query{ url_query_builder{}
			.add_parameter(_constants.queries.STATUS, DONE)
			.to_string() };

		return send_private_request<std::vector<order_description>>(http_verb::GET, path, coinbase::read_orders, query);
	}

	std::string coinbase_api::add_order(const trade_description& description)
	{
		static constexpr std::string_view TYPE = "type";
		static constexpr std::string_view SIDE = "side";
		static constexpr std::string_view PRODUCT_ID = "product_id";
		static constexpr std::string_view PRICE = "price";
		static constexpr std::string_view SIZE = "size";
		static constexpr std::string_view STOP = "stop";
		static constexpr std::string_view STOP_PRICE = "stop_price";

		std::string path{ build_url_path(std::array<std::string_view, 1>
		{
			_constants.methods.ORDERS
		}) };

		json_writer json = json_writer{}
			.add(SIDE, to_string(description.action()))
			.add(PRODUCT_ID, internal::to_exchange_id(description.pair()))
			.add(SIZE, std::to_string(description.volume()));

		if (is_stop_order(description.order_type()))
		{
			json.add(TYPE, ::to_string(order_type::LIMIT));
			json.add(PRICE, std::to_string(description.asset_price()));
			json.add(STOP, ::to_string(description.order_type()));
			json.add(STOP_PRICE, std::to_string(description.asset_price()));
		}
		else
		{
			json.add(TYPE, ::to_string(description.order_type()));

			if (description.order_type() != order_type::MARKET)
			{
				json.add(PRICE, std::to_string(description.asset_price()));
			}
		}

		std::string content{ json.to_string() };

		return send_private_request<std::string>(http_verb::POST, path, coinbase::read_add_order, "", content);
	}

	void coinbase_api::cancel_order(std::string_view orderId)
	{
		std::string path{ build_url_path(std::array<std::string_view, 2>
		{
			_constants.methods.ORDERS,
			orderId
		}) };

		send_private_request<void>(http_verb::HTTP_DELETE, path, coinbase::read_cancel_order);
	}

	std::unique_ptr<exchange> make_coinbase(coinbase_config config, std::shared_ptr<websocket_client> websocketClient, bool enableTesting)
	{
		return std::make_unique<coinbase_api>(
			std::move(config),
			std::make_unique<http_service>(),
			std::make_unique<exchange_websocket_stream>(std::make_unique<internal::coinbase_websocket_stream>(), websocketClient),
			enableTesting);
	}
}