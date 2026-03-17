# Timestamps

Wu uses a flexible timestamp representation that supports different time
granularities without conversion overhead.

## Time Representation

Every data point in Wu carries a `WU_TimeStamp`:

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

The `mark` field holds the actual time value. The `units` field specifies
the scale. This design avoids forcing a single time representation across
the entire system.

## Usage Patterns

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

**High-frequency data** can use microseconds or nanoseconds for tick-level
precision:

```c
WU_TimeStamp ts = {
    .mark = 1678896000000000000,  // Nanoseconds since epoch
    .units = WU_TIME_UNIT_NANOS
};
```

## Time Arithmetic

When calculating time-based metrics, Wu converts timestamps to a common
unit internally. For example, calculating borrowing interest on short
positions requires knowing how long the position was held.

The calculation process:

1. Check that both timestamps use the same unit
2. Compute the difference in that unit
3. Convert to seconds
4. Convert seconds to years (dividing by 365.25 × 86400)
5. Apply the annual rate

This conversion happens inside the portfolio implementation. Users only
need to ensure their data uses consistent time units.

## CSV Reader Time Handling

The CSV reader accepts a `WU_TimeUnit` parameter at construction:

```c
WU_Reader reader = wu_csv_reader_new(
    file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_SECONDS,  // Interpret timestamps as seconds
    true                    // Has header row
);
```

The reader parses the timestamp column as `int64_t` and wraps it with the
specified unit. If your CSV uses Unix seconds, specify SECONDS. If it
uses milliseconds, specify MILLIS.

## Time Units Consistency

All data fed into a strategy should use the same time unit. The runner
doesn't enforce this, but mixing units produces incorrect time-based
calculations.

If you have data in different units, convert them when creating readers
or preprocess the CSV files to use consistent timestamps.

## Precision and Range

Using `int64_t` provides large range:

- **Seconds**: Covers 292 billion years from epoch
- **Milliseconds**: Covers 292 million years from epoch  
- **Microseconds**: Covers 292 thousand years from epoch
- **Nanoseconds**: Covers 292 years from epoch (1970-2262)

For trading applications, even nanosecond precision has sufficient range
to cover any realistic backtest period.

## Design Rationale

**Why not always use nanoseconds?** Different data sources use different
scales. Daily data from many providers uses Unix seconds. Converting
everything to a single scale requires overhead and potential precision
loss. Letting each data source keep its natural representation is simpler.

**Why not floating point?** Integer arithmetic is exact. No rounding
errors when computing differences. No precision loss when comparing
timestamps. The unit field handles scale explicitly rather than relying
on implicit floating-point magnitude.

**Why store the unit?** It makes the representation self-describing. When
debugging, you can see immediately whether a timestamp is in seconds or
milliseconds. It also enables runtime validation that operations on
timestamps use compatible units.

## Limitations

**No timezone handling**: All timestamps are assumed to be in a
consistent timezone. Wu doesn't do timezone conversion or daylight saving
time adjustments. Preprocess your data to use UTC or a consistent local
time.

**No date arithmetic**: Wu doesn't parse or format dates. It works with
integer time values. Use external tools to convert between human-readable
dates and Unix timestamps.

**No calendar awareness**: The year conversion (dividing by 365.25 days)
is approximate. It doesn't account for leap seconds, market holidays, or
trading hour adjustments. For borrowing cost calculations this is
acceptable, but for precise calendar arithmetic you'd need additional
logic.

## Future Considerations

The current implementation is adequate for typical backtesting. Potential
enhancements could include:

- Validation that all data sources use compatible units
- Helper functions for unit conversion
- Support for relative time (bars since start) vs absolute time
- Trading calendar integration for market hours

These remain unexplored for now. The current approach handles common
cases without unneeded complexity.
