# Timestamps and Time Units

Timestamps are fundamental to backtesting. They determine when data occurred, enable proper historical ordering, and affect how metrics like Sharpe ratio get annualized. This guide explains Wu's timestamp model from both user and implementation perspectives.

## Time Representation

Wu uses a flexible timestamp representation that supports different time granularities without conversion overhead.

### The WU_TimeStamp Structure

```c
typedef enum {
    WU_TIME_UNIT_SECONDS = 0,
    WU_TIME_UNIT_MILLIS = 1,
    WU_TIME_UNIT_MICROS = 2,
    WU_TIME_UNIT_NANOS = 3
} WU_TimeUnit;

typedef struct WU_TimeStamp_ {
    int64_t mark;
    WU_TimeUnit units;
} WU_TimeStamp;
```

The `mark` field holds the actual time value. The `units` field specifies the scale. This design avoids forcing a single time representation across the entire system.

### Why This Design?

**Why not always use nanoseconds?** Different data sources use different scales. Daily data from many providers uses Unix seconds. Converting everything to a single scale requires overhead and potential precision loss. Letting each data source keep its natural representation is simpler.

**Why not floating point?** Integer arithmetic is exact. No rounding errors when computing differences. No precision loss when comparing timestamps. The unit field handles scale explicitly rather than relying on implicit floating-point magnitude.

**Why store the unit?** It makes the representation self-describing. When debugging, you can see immediately whether a timestamp is in seconds or milliseconds. It also enables runtime validation that operations on timestamps use compatible units.

---

## Understanding Time Units

Wu always manages Unix timestamps (time since January 1, 1970 UTC), but you specify the unit in which these timestamps are expressed. The same timestamp value can represent different moments in time depending on its unit.

### Time Unit Examples

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

### Usage Patterns by Data Frequency

**Daily data** typically uses seconds with Unix epoch timestamps:

```c
WU_TimeStamp ts = {
    .mark = 1678896000,  // Unix timestamp for March 15, 2023
    .units = WU_TIME_UNIT_SECONDS
};
```

**Intraday data** might use milliseconds for minute or second bars:

```c
WU_TimeStamp ts = {
    .mark = 1678896000000,  // Milliseconds since epoch
    .units = WU_TIME_UNIT_MILLIS
};
```

**High-frequency data** can use microseconds or nanoseconds for tick-level precision:

```c
WU_TimeStamp ts = {
    .mark = 1678896000000000000,  // Nanoseconds since epoch
    .units = WU_TIME_UNIT_NANOS
};
```

---

## Unix Timestamps in Practice

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

---

## Data Frequencies

Your timestamps are always Unix values, but the frequency at which you receive data differs:

### Daily Bars

One candle per trading day. Timestamps typically represent market close time (4 PM ET for US equities).

```c
// 1704067200 = Jan 1, 2024 00:00:00 UTC
// 1704153600 = Jan 2, 2024 00:00:00 UTC  (1 day = 86400 seconds later)
WU_TimeUnit time_unit = WU_TIME_UNIT_SECONDS;
```

The delta between consecutive timestamps is approximately 86400 seconds (one trading day).

### Hourly Bars

One candle per hour. Useful for intraday strategies.

```c
// 1704067200 = Jan 1, 2024 00:00:00 UTC
// 1704070800 = Jan 1, 2024 01:00:00 UTC  (1 hour = 3600 seconds later)
WU_TimeUnit time_unit = WU_TIME_UNIT_SECONDS;
```

### Minute Data

One candle per minute.

```c
// 1704067200 = Jan 1, 2024 00:00:00 UTC
// 1704067260 = Jan 1, 2024 00:01:00 UTC  (1 minute = 60 seconds later)
WU_TimeUnit time_unit = WU_TIME_UNIT_SECONDS;
```

## Time Arithmetic and Calculations

When calculating time-based metrics, Wu converts timestamps to a common unit internally. For example, calculating borrowing interest on short positions requires knowing how long the position was held.

### The Calculation Process

1. Check that both timestamps use the same unit
2. Compute the difference in that unit
3. Convert to seconds
4. Convert seconds to years (dividing by 365.25 × 86400)
5. Apply the annual rate

This conversion happens inside the portfolio implementation. Users only need to ensure their data uses consistent time units.

## Limitations

**No timezone handling**: All timestamps are assumed to be in a consistent timezone. Wu doesn't do timezone conversion or daylight saving time adjustments. Preprocess your data to use UTC or a consistent local time.

**No date arithmetic**: Wu doesn't parse or format dates. It works with integer time values. Use external tools to convert between human-readable dates and Unix timestamps.

**No calendar awareness**: The year conversion (dividing by 365.25 days) is approximate. It doesn't account for leap seconds, market holidays, or trading hour adjustments. For borrowing cost calculations this is acceptable, but for precise calendar arithmetic you'd need additional logic.
