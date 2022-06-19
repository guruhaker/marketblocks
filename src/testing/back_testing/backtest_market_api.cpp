#include "backtest_market_api.h"

namespace mb
{
	backtest_market_api::backtest_market_api(std::shared_ptr<back_testing_data_navigator> dataSource, std::unique_ptr<backtest_websocket_stream> websocketStream)
	: _dataNavigator{ std::move(dataSource) }, _websocketStream{ std::move(websocketStream) }
	{
	}

	void backtest_market_api::increment_data()
	{ 
		_dataNavigator->increment_data();
	}

	exchange_status backtest_market_api::get_status() const
	{
		return exchange_status::ONLINE;
	}

	std::vector<tradable_pair> backtest_market_api::get_tradable_pairs() const
	{
		return _dataNavigator->data().tradable_pairs();
	}

	ohlcv_data backtest_market_api::get_24h_stats(const tradable_pair& tradablePair) const
	{
		return _dataNavigator->get_merged_ohlcv(tradablePair, 86400);
	}

	double backtest_market_api::get_price(const tradable_pair& tradablePair) const
	{
		return _websocketStream->get_price(tradablePair);
	}

	order_book_state backtest_market_api::get_order_book(const tradable_pair& tradablePair, int depth) const
	{
		return _websocketStream->get_order_book(tradablePair, depth);
	}
}