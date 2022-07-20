#pragma once

#include "common/json/json.h"
#include "exchanges/websockets/exchange_websocket_stream.h"

namespace mb::internal
{
	class binance_websocket_stream : public exchange_websocket_stream
	{
	private:
		void process_trade_message(const json_document& json);
		void process_ohlcv_message(const json_document& json);

		void on_message(std::string_view message) override;
		void send_subscribe(const websocket_subscription& subscription) override;
		void send_unsubscribe(const websocket_subscription& subscription) override;

	public:
		binance_websocket_stream(std::unique_ptr<websocket_connection_factory> connectionFactory);
	};
}