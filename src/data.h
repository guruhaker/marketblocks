#pragma once

class PriceData
{
private:
	const double _bid;
	const double _ask;

public:
	explicit PriceData()
		: _bid{0}, _ask{0}
	{}

	explicit PriceData(double bid, double ask)
		: _bid{bid}, _ask{ask}
	{}

	double bid() const { return _bid; }
	double ask() const { return _ask; }
};