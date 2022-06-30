#pragma once

#include <filesystem>
#include <format>

namespace mb::test
{
	static constexpr std::string_view TEST_DATA_FOLDER = "test_data";

	std::filesystem::path kraken_results_test_data_path(std::string_view fileName);
	std::filesystem::path kraken_websocket_test_data_path(std::string_view fileName);
	std::filesystem::path coinbase_results_test_data_path(std::string_view fileName);
	std::filesystem::path bybit_results_test_data_path(std::string_view fileName);
	std::filesystem::path digifinex_results_test_data_path(std::string_view fileName);
	std::filesystem::path csv_test_data_path(std::string_view fileName);

	std::filesystem::path csv_back_test_data_directory();
}