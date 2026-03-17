# Understanding Timestamps and Data Frequencies

Timestamps are fundamental to backtesting. They determine when data occurred, enable proper historical ordering, and affect how metrics like Sharpe ratio get annualized. This guide explains Wu's timestamp model.

## Time Units

Wu always manages Unix timestamps (time since January 1, 1970 UTC), but you specify the unit in which these timestamps are expressed. The same timestamp value can represent different moments in time depending on its unit.

```c
typedef enum {
    WU_TIME_UNIT_SECONDS = 0,
    WU_TIME_UNIT_MILLIS = 1,
    WU_TIME_UNIT_MICROS = 2,
    WU_TIME_UNIT_NANOS = 3
} WU_TimeUnit;
```

For example, the same timestamp value `1704067200` means:
- 1704067200 seconds = January 1, 2024 00:00:00 UTC
- 1704067200 milliseconds = January 20, 1970 18:01:07 UTC (almost immediately after epoch)
- 1704067200 microseconds = January 1, 1970 00:28:24 UTC

When you create a data reader, you specify the time unit so Wu can properly interpret your timestamps:

```c
WU_Reader reader = (WU_Reader)wu_csv_reader_new(file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_SECONDS,    // Your timestamps are in Unix seconds
    true                      // Skip header row
);
```

This tells Wu how to interpret the numeric timestamps in your data and allows it to properly scale time-based metrics like Sharpe ratio and volatility calculations.

## Unix Timestamps

Most commonly, timestamps are Unix epoch values—seconds since January 1, 1970 UTC. This is what financial data tools like `yf` produce with `--date_format:unix`.

For example:
- January 1, 2024 00:00:00 UTC = 1704067200
- January 1, 2020 00:00:00 UTC = 1577836800
- December 31, 2010 00:00:00 UTC = 1293840000

**CSV Format**:

```
Timestamp,Open,High,Low,Close,Volume
1577836800,100.5,101.2,100.0,100.8,1000000
1577923200,100.8,101.5,100.5,101.2,1100000
1578009600,101.2,102.0,100.8,101.8,950000
```

The timestamps should be in the first column. Wu's CSV reader parses them as numeric values and combines them with the time unit you specified.

## Data Frequencies

Your timestamps are always Unix values, but the frequency at which you receive data differs:

**Daily Bars**: One candle per trading day. Timestamps typically represent market close time (4 PM ET for US equities).

```c
// 1704067200 = Jan 1, 2024 00:00:00 UTC
// 1704153600 = Jan 2, 2024 00:00:00 UTC  (1 day = 86400 seconds later)
WU_TimeUnit time_unit = WU_TIME_UNIT_SECONDS;
```

The delta between consecutive timestamps is approximately 86400 seconds (one trading day).

**Hourly Bars**: One candle per hour. Useful for intraday strategies.

```c
// 1704067200 = Jan 1, 2024 00:00:00 UTC
// 1704070800 = Jan 1, 2024 01:00:00 UTC  (1 hour = 3600 seconds later)
WU_TimeUnit time_unit = WU_TIME_UNIT_SECONDS;
```

**Minute Data**: One candle per minute.

```c
// 1704067200 = Jan 1, 2024 00:00:00 UTC
// 1704067260 = Jan 1, 2024 00:01:00 UTC  (1 minute = 60 seconds later)
WU_TimeUnit time_unit = WU_TIME_UNIT_SECONDS;
```

**Tick Data**: Individual transactions at millisecond or microsecond precision.

```c
// Use milliseconds if your data comes at that granularity
WU_TimeUnit time_unit = WU_TIME_UNIT_MILLIS;

// Or microseconds for very high-frequency data
WU_TimeUnit time_unit = WU_TIME_UNIT_MICROS;
```

Always use the same time unit across your entire backtest and dataset. Wu will properly scale time-dependent metrics based on the unit you specify.

## Performance Metrics and Annualization

Risk metrics like Sharpe ratio and Sortino ratio depend on proper time scaling. A Sharpe ratio of 1.0 on daily data means something different from one on hourly data.

Wu automatically scales these metrics based on the time unit of your data. The library tracks the time span of your backtest and annualizes metrics to a standard year:

```c
WU_SharpeRatio sharpe = wu_sharpe_ratio_new();
// Wu annualizes based on WU_TIME_UNIT_SECONDS (252 trading days/year × 86400 seconds/day)
```

The annualization factors are:

- **Daily data** (WU_TIME_UNIT_SECONDS, ~86400 sec intervals): 252 trading days per year
- **Hourly data** (WU_TIME_UNIT_SECONDS, ~3600 sec intervals): 252 × 6.5 = 1,638 trading hours per year
- **Minute data** (WU_TIME_UNIT_SECONDS, ~60 sec intervals): 252 × 6.5 × 60 = 98,280 trading minutes per year

These conventions (252 trading days/year, 6.5 trading hours/day) are standard in finance.

**Why This Matters**:

Consider two strategies with identical 10% returns:

1. Daily strategy over 252 trading days (1 year of daily bars)
2. Minute-level strategy over the same calendar period (98,280 minute bars)

Both show 10% returns, but the minute strategy has many more opportunities to generate returns. Without proper time scaling, you'd compare them incorrectly. Wu handles this automatically once you specify the time unit correctly.

## Example: Converting Data Granularity

Suppose you have minute-level data and want to aggregate it to daily bars:

```bash
# Raw minute data (raw_data.csv)
1577836800,100.5,101.2,100.0,100.8,1000
1577836860,100.8,101.5,100.5,101.2,1100
1577836920,101.2,102.0,100.8,101.8,950
... (390 more minutes in this day) ...
1577923200,100.9,101.3,100.1,101.0,1050

# After aggregation into daily bars (daily_data.csv)
1577836800,100.5,102.1,100.0,101.0,500000
1577923200,101.0,102.5,100.5,101.8,480000
```

The aggregation process groups minute bars and computes:
- Open: first minute's open
- High: maximum high across all minutes
- Low: minimum low across all minutes
- Close: last minute's close
- Volume: sum of volumes

Then use the aggregated file with the correct time unit:

```c
WU_Reader reader = wu_csv_reader_new(file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_SECONDS,    // Time unit matches your data (daily bars)
    true
);
```

Wu will properly annualize metrics based on daily bar frequency (252 trading days per year).

## Practical Considerations

### Data Alignment

Ensure your timestamps are in the time unit you specify:

```c
// If your CSV has daily bars with Unix timestamps in seconds:
WU_TimeUnit time_unit = WU_TIME_UNIT_SECONDS;

// If your CSV has tick data with timestamps in milliseconds:
WU_TimeUnit time_unit = WU_TIME_UNIT_MILLIS;
```

Mismatch between your data and specified unit causes incorrect metric calculations.

### Market Hours

Daily data typically timestamps bars at market close (4 PM ET for US equities). Some tools use market open. Ensure consistency across all data sources.

```bash
# Using yf to download data with Unix timestamps
yf history -s:spy --lookback:1y --format:csv --date_format:unix

# Produces timestamps at market close time (4 PM ET)
# 1704067200 = Jan 1, 2024 00:00 UTC
```

### UTC vs Local Time

Wu uses UTC internally. If your data is in local time, convert it first:

```bash
# EST/EDT local time to UTC
# EST is UTC-5, EDT is UTC-4

# 1:00 PM EST = 18:00 UTC
# 1:00 PM EDT = 17:00 UTC
```

Financial tools like `yf` output UTC by default, so this is usually not an issue.

### Gaps in Data

Markets are closed on weekends and holidays. Daily data has gaps:

```
1704067200  (Friday close)
1704240000  (Monday close, 2 calendar days later)
```

This is normal. Wu handles gaps correctly. Gaps don't affect statistics—they just mean fewer periods elapsed.

Intraday data (hourly or minute) shouldn't have gaps during trading hours unless trading 24/7 futures.

## Tips for Setting Up Data

1. **Verify Your Timestamps**: Print the first few and last few rows to confirm they're in the right order and frequency.

2. **Match Time Units**: The time unit you pass to `wu_csv_reader_new` must match your data's granularity.

3. **Use Unix Epoch**: It's the standard. Tools like `yf` produce it directly.

4. **Handle Timezone**: Ensure all your data is in the same timezone (preferably UTC).

5. **Document Your Data**: Note the frequency, timezone, and whether bars close at market close or open.

## Example Workflow

```bash
# 1. Pull data in the right format
yf history -s:spy --lookback:5y \
    --format:csv \
    --date_format:unix > spy.csv

# 2. Verify the format
head -5 spy.csv
# Output:
# Timestamp,Open,High,Low,Close,Volume
# 1577836800,100.5,101.2,100.0,100.8,1000000
# ...

# 3. Use in Wu with correct time unit
// In your C code:
WU_Reader reader = wu_csv_reader_new(file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_DAYS,    // Daily bars
    true                  // Skip header
);

// Performance metrics are now properly annualized
```

## See Also

For API details, see the [Doxygen API Reference](https://jailop.codeberg.page/wutrader/docs/html/).

For a complete working example with real data, see the [Tutorial](../tutorial.md).
