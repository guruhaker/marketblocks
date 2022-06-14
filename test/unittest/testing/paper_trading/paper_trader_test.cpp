#include <gtest/gtest.h>

#include "testing/paper_trading/paper_trade_api.h"
#include "common/exceptions/mb_exception.h"
#include "common/utils/containerutils.h"

namespace
{
	using namespace mb;

	class get_prices
	{
	private:
		std::unordered_map<tradable_pair, double> _prices;

	public:
		get_prices(std::unordered_map<tradable_pair, double> prices)
			: _prices{ std::move(prices) }
		{}

		void set_price(tradable_pair pair, double price)
		{
			_prices.insert_or_assign(std::move(pair), price);
		}

		double get_price(const tradable_pair& pair) const
		{
			return find_or_default(_prices, pair, 0.0);
		}
	};

	paper_trade_api create_paper_trade_api(
		double initialGbpBalance,
		double initialBtcBalance,
		double fee,
		const get_prices& getPrices)
	{
		unordered_string_map<double> initialBalances
		{
			{ "GBP", initialGbpBalance },
			{ "BTC", initialBtcBalance }
		};

		return paper_trade_api{
			paper_trading_config{ fee, std::move(initialBalances) },
			"",
			[&getPrices](const tradable_pair& pair) {return getPrices.get_price(pair); } };
	}
}

namespace mb::test
{
	TEST(PaperTrader, AddBuyOrderCorrectlyAdjustsBalances)
	{
		constexpr double initialGbpBalance = 100.0;
		constexpr double initialBtcBalance = 1.5;
		constexpr double fee = 0.1;
		constexpr double assetPrice = 20.0;
		constexpr double volume = 2.0;

		tradable_pair pair{ "BTC", "GBP" };
		get_prices prices{{ { pair, assetPrice } }};

		paper_trade_api trader{ create_paper_trade_api(initialGbpBalance, initialBtcBalance, fee, prices) };

		trade_description tradeDescription
		{
			order_type::LIMIT,
			pair,
			trade_action::BUY,
			assetPrice,
			volume
		};

		constexpr double expectedGbpBalance = 59.96;
		constexpr double expectedBtcBalance = 3.5;

		std::string orderId = trader.add_order(tradeDescription);

		unordered_string_map<double> balances = trader.get_balances();

		EXPECT_EQ(orderId, "1");
		EXPECT_DOUBLE_EQ(balances.at(tradeDescription.pair().asset()), expectedBtcBalance);
		EXPECT_DOUBLE_EQ(balances.at(tradeDescription.pair().price_unit()), expectedGbpBalance);
	}

	TEST(PaperTrader, AddSellOrderCorrectlyAdjustsBalances)
	{
		constexpr double initialGbpBalance = 100.0;
		constexpr double initialBtcBalance = 1.5;
		constexpr double fee = 0.1;
		constexpr double assetPrice = 20.0;
		constexpr double volume = 1.0;

		tradable_pair pair{ "BTC", "GBP" };
		get_prices prices{ { { pair, assetPrice } } };

		paper_trade_api trader{ create_paper_trade_api(initialGbpBalance, initialBtcBalance, fee, prices) };

		trade_description tradeDescription
		{
			order_type::LIMIT,
			pair,
			trade_action::SELL,
			assetPrice,
			volume
		};

		constexpr double expectedGbpBalance = 119.98;
		constexpr double expectedBtcBalance = 0.5;

		std::string orderId = trader.add_order(tradeDescription);

		unordered_string_map<double> balances = trader.get_balances();

		EXPECT_EQ(orderId, "1");
		EXPECT_DOUBLE_EQ(balances.at(tradeDescription.pair().asset()), expectedBtcBalance);
		EXPECT_DOUBLE_EQ(balances.at(tradeDescription.pair().price_unit()), expectedGbpBalance);
	}

	TEST(PaperTrader, AddBuyOrderThrowsIfInsufficientFunds)
	{
		constexpr double initialGbpBalance = 20.0;
		constexpr double initialBtcBalance = 1.5;
		constexpr double fee = 0.1;
		constexpr double assetPrice = 50.0;
		constexpr double volume = 1.0;

		tradable_pair pair{ "BTC", "GBP" };
		get_prices prices{ { { pair, assetPrice } } };

		paper_trade_api trader{ create_paper_trade_api(initialGbpBalance, initialBtcBalance, fee, prices) };

		trade_description tradeDescription
		{
			order_type::LIMIT,
			pair,
			trade_action::BUY,
			assetPrice,
			volume
		};

		EXPECT_THROW(trader.add_order(tradeDescription), mb_exception);
	}

	TEST(PaperTrader, AddSellOrderThrowsIfInsufficientFunds)
	{
		constexpr double initialGbpBalance = 100.0;
		constexpr double initialBtcBalance = 0.5;
		constexpr double fee = 0.1;
		constexpr double assetPrice = 50.0;
		constexpr double volume = 1.0;

		tradable_pair pair{ "BTC", "GBP" };
		get_prices prices{ { { pair, assetPrice } } };

		paper_trade_api trader{ create_paper_trade_api(initialGbpBalance, initialBtcBalance, fee, prices) };

		trade_description tradeDescription
		{
			order_type::LIMIT,
			pair,
			trade_action::SELL,
			assetPrice,
			volume
		};

		EXPECT_THROW(trader.add_order(tradeDescription), mb_exception);
	}

	TEST(PaperTrader, AddLimitOrderDoesNotExecuteIfPriceNotReached)
	{
		constexpr double initialGbpBalance = 100.0;
		constexpr double initialBtcBalance = 1.5;
		constexpr double fee = 0.1;
		constexpr double assetPrice = 40.0;
		constexpr double volume = 2.0;
		constexpr double orderPrice = 20.0;

		tradable_pair pair{ "BTC", "GBP" };
		get_prices prices{ { { pair, assetPrice } } };

		paper_trade_api trader{ create_paper_trade_api(initialGbpBalance, initialBtcBalance, fee, prices) };

		trade_description tradeDescription
		{
			order_type::LIMIT,
			pair,
			trade_action::BUY,
			orderPrice,
			volume
		};

		trader.add_order(tradeDescription);

		unordered_string_map<double> balances = trader.get_balances();

		EXPECT_DOUBLE_EQ(balances.at(tradeDescription.pair().asset()), initialBtcBalance);
		EXPECT_DOUBLE_EQ(balances.at(tradeDescription.pair().price_unit()), initialGbpBalance);
	}

	TEST(PaperTrader, CloseOpenOrdersClosesOrders)
	{
		constexpr double initialGbpBalance = 100.0;
		constexpr double initialBtcBalance = 1.5;
		constexpr double fee = 0.1;
		constexpr double assetPrice = 40.0;
		constexpr double volume = 2.0;
		constexpr double orderPrice = 20.0;

		tradable_pair pair{ "BTC", "GBP" };
		get_prices prices{ { { pair, assetPrice } } };

		paper_trade_api trader{ create_paper_trade_api(initialGbpBalance, initialBtcBalance, fee, prices) };

		trade_description tradeDescription1
		{
			order_type::LIMIT,
			pair,
			trade_action::BUY,
			orderPrice,
			volume
		};

		trade_description tradeDescription2
		{
			order_type::LIMIT,
			pair,
			trade_action::BUY,
			orderPrice / 2,
			volume
		};

		trader.add_order(tradeDescription1);
		trader.add_order(tradeDescription2);

		prices.set_price(pair, orderPrice);
		trader.fill_open_orders();

		constexpr double expectedGbpBalance = 59.96;
		constexpr double expectedBtcBalance = 3.5;

		unordered_string_map<double> balances = trader.get_balances();

		EXPECT_DOUBLE_EQ(balances.at(pair.asset()), expectedBtcBalance);
		EXPECT_DOUBLE_EQ(balances.at(pair.price_unit()), expectedGbpBalance);
	}
}