# Understanding Timestamps and Data Frequencies

Timestamps are fundamental to backtesting. They determine when data occurred, enable proper historical ordering, and affect how metrics like Sharpe ratio get annualized. This guide explains Wu's timestamp model.

## Time Units

Wu represents time using a flexible unit system. The same numeric value can represent seconds, minutes, days, or years depending on the context.

```c
typedef enum {
    WU_TIME_UNIT_SECONDS,
    WU_TIME_UNIT_MINUTES,
    WU_TIME_UNIT_HOURS,
    WU_TIME_UNIT_DAYS,
    WU_TIME_UNIT_WEEKS,
    WU_TIME_UNIT_MONTHS,
    WU_TIME_UNIT_YEARS
} WU_TimeUnit;
```

When you create a data reader, you specify the time unit:

```c
WU_Reader reader = (WU_Reader)wu_csv_reader_new(file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_SECONDS,    // Timestamps are in Unix seconds
    true                      // Skip header row
);
```

This tells Wu how to interpret the numeric timestamps in your data.

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

## Bar Frequencies and Data Types

Different data frequencies require different interpretations:

**Daily Bars**: One data point per calendar day (or trading day).

```c
// Data file with daily bars
WU_TimeUnit time_unit = WU_TIME_UNIT_DAYS;
```

Your timestamps might be daily Unix values (at market close or market open). The delta between consecutive timestamps is roughly 86400 seconds (one day).

**Hourly Bars**: One data point per hour.

```c
WU_TimeUnit time_unit = WU_TIME_UNIT_HOURS;
```

Timestamps progress hourly. More granular than daily data, useful for intraday strategies.

**Tick Data**: Individual transactions.

```c
WU_TimeUnit time_unit = WU_TIME_UNIT_SECONDS;
```

Highest granularity. Every price change has its own timestamp. Tick data is much larger and computationally expensive to process.

## Performance Metrics and Annualization

Risk metrics like Sharpe ratio and Sortino ratio are meaningless without proper time scaling. A Sharpe ratio of 1.0 on daily data isn't the same as a Sharpe ratio of 1.0 on hourly data.

Wu automatically annualizes these metrics based on the time unit you specify:

```c
WU_SharpeRatio sharpe = wu_sharpe_ratio_new(initial_value, risk_free_rate);
```

Internally, Wu tracks how many periods occurred during the backtest and scales the metrics accordingly. The calculation uses:

```
periods_per_year = 252 for daily data
                 = 252 * 6.5 = 1638 for hourly data
                 = 252 * 6.5 * 60 = 98280 for minute data
```

These conventions (252 trading days/year, 6.5 trading hours/day) are standard in finance.

**Why This Matters**:

Consider two strategies:

1. Daily strategy over 252 trading days (1 year)
2. Hourly strategy over 252 days × 6.5 hours/day = 1,638 hourly bars

Both might show 10% returns. But the hourly strategy has 6.5× more opportunities to generate returns, so the daily volatility is higher. Without proper annualization, you'd compare returns at different scales.

Wu handles this automatically once you specify the time unit correctly.

## Time Unit Example: Converting Data

Suppose you have minute-level data but want to test on daily bars. You'd aggregate:

```bash
# Raw minute data (raw_data.csv)
1577836800,100.5,101.2,100.0,100.8,1000
1577836860,100.8,101.5,100.5,101.2,1100
1577836920,101.2,102.0,100.8,101.8,950
... (390 more minutes) ...
1577923200,100.9,101.3,100.1,101.0,1050

# After aggregation into daily bars (daily_data.csv)
1577836800,100.5,102.1,100.0,101.0,500000
1577923200,101.0,102.5,100.5,101.8,480000
```

The aggregation process:
- Groups minutes into days
- Uses the first minute's open as the day's open
- Tracks the highest high and lowest low across all minutes
- Uses the last minute's close as the day's close
- Sums volumes

Then when you use this daily file:

```c
WU_Reader reader = wu_csv_reader_new(file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_DAYS,    // Time unit matches the data
    true
);
```

## Practical Considerations

### Data Alignment

Ensure timestamps align with your data frequency:

```c
// If your CSV has daily bars, timestamps should be:
// - Daily Unix values (e.g., 1577836800, 1577923200, ...)
// - Time unit: WU_TIME_UNIT_DAYS

// If your CSV has hourly bars, timestamps should be:
// - Hourly Unix values (incremented by 3600 seconds)
// - Time unit: WU_TIME_UNIT_HOURS
```

Mismatch causes incorrect metric calculations.

### Market Hours

Daily data typically timestamps bars at market close (4 PM ET for US equities). Some tools timestamp at market open. Either way, ensure consistency across all your data sources.

```bash
# Market close convention (4 PM ET)
yf history -s:spy --lookback:1y --format:csv --date_format:unix

# Produces timestamps at market close time
# 1704067200 = Jan 1, 2024 00:00 UTC (coincidentally Jan 1 at noon ET)
```

### UTC vs Local Time

Wu uses UTC internally. If your data is in local time, convert it first:

```bash
# EST/EDT local time to UTC
# EST is UTC-5, EDT is UTC-4

# 1:00 PM EST = 18:00 UTC
# 1:00 PM EDT = 17:00 UTC
```

Financial tools like `yf` output UTC by default, so usually this isn't an issue.

### Gaps in Data

Markets are closed on weekends and holidays. Your daily data will have gaps:

```
1704067200  (Friday)
1704240000  (Monday, 2 days later)
```

This is fine. Wu handles gaps correctly. Gaps don't affect statistics calculations—they just mean fewer periods elapsed.

However, if you're working with intraday data (hourly or minute), gaps are more noticeable. You shouldn't have timestamps spanning non-trading hours unless you're trading 24/7 futures.

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

For a complete working example with real data, see the [Tutorial](tutorial.md).
