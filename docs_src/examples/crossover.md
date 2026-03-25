# Crossover Strategy Example

This example demonstrates a simple moving average crossover strategy on a
single asset. The strategy generates buy signals when a fast moving
average crosses above a slow moving average, and sell signals on the
reverse.

## The Strategy

Moving average crossover is a basic trend-following approach. Two moving
averages track price trends at different timescales. When the fast
average crosses above the slow average, it suggests upward momentum
(buy). When it crosses below, it suggests downward momentum (sell).

This strategy doesn't predict where prices will go. It follows trends
after they've started, hoping to capture some of the move before they
reverse.

## Code Structure

The example in `examples/backtest/example01.c` shows the complete flow:

```c
// Open data file
FILE* file = fopen(argv[1], "r");
WU_Reader reader = wu_csv_reader_new(
    file, 
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_SECONDS,
    true
);

// Create crossover strategy (10-period fast, 30-period slow)
WU_Strategy strategy = wu_crossover_strat_new(10, 30, 0.0);

// Configure portfolio
WU_PortfolioParams params = {
    .direction = WU_DIRECTION_LONG,
    .initial_cash = 100000.0,
    .execution_policy = {
        .policy = WU_EXECUTION_POLICY_FIXED_SLIPPAGE,
        .execution_mean = 0.0005,
        .execution_stddev = 0.0,
        .tx_cost_type = WU_TRANSACTION_COST_PROPORTIONAL,
        .tx_cost_value = 0.001,
        .stop_loss_pct = 0.10,
        .take_profit_pct = 0.20
    },
    .borrow_params = {
        .rate = 0.0,
        .limit = 0.0
    },
    .position_sizing = {
        .size_type = WU_POSITION_SIZE_PCT,
        .size_value = 1.0
    }
};

WU_BasicPortfolio portfolio = wu_basic_portfolio_new(
    params,
    wu_symbol_list("ASSET")
);

// Run backtest
WU_Runner runner = wu_runner_new(
    WU_PORTFOLIO(portfolio),
    WU_STRATEGY(strategy),
    wu_reader_list(WU_READER(reader))
);

wu_runner_exec(runner, false);  // false = silent mode
```

## Configuration Details

**Strategy parameters**: The 10-period and 30-period windows determine
responsiveness. Shorter windows react faster to price changes but produce
more false signals. Longer windows are smoother but lag more. The third
parameter (0.0) is reserved for future threshold features.

**Portfolio sizing**: Uses 100% of cash (`size_value = 1.0`), making this
an all-in strategy. When the fast MA crosses above slow MA, the entire
cash balance buys the asset. When it crosses below, the entire position
sells.

**Risk management**: Stop loss at 10% limits downside on individual
trades. Take profit at 20% locks in gains. These exits trigger
independently of strategy signals.

**Execution costs**: 0.05% slippage and 0.1% transaction cost per trade.
On a $100k position, that's $150 total cost. Over many trades, these
costs accumulate and significantly impact returns.

## Running the Example

```bash
# Build
make

# Run on test data
./examples/backtest/example01 ./tests/data/spy.csv

# Run in verbose mode to see each trade
./examples/backtest/example01 ./tests/data/spy.csv -v
```

Verbose mode shows:

```
[BUY] 2020-03-23 -> 2020-03-24 | Price: 232.44 -> 241.20 | Quantity: 430.4326 | PnL: 3770.62 (3.77%)
[SELL] 2020-03-24 -> 2020-04-09 | Price: 241.20 -> 274.40 | Quantity: 430.4326 | PnL: 14290.02 (13.78%)
```

## Output Interpretation

The strategy reports:

- Total PnL and percentage return
- Number of trades with win/loss breakdown
- Maximum win and maximum loss
- Transaction fees and borrowing costs (0 for long-only)
- Final position (quantity held, value, last price)

These metrics give a basic picture of strategy performance. They don't
include risk metrics like Sharpe ratio, maximum drawdown, or volatility.

## Limitations

**Whipsaw risk**: In choppy markets, the moving averages cross repeatedly
without meaningful trends. Each cross generates a trade, accumulating
costs without profit. This strategy performs poorly in ranging markets.

**Lag**: By the time moving averages cross, much of the trend has already
passed. You enter late and exit late. In fast-moving markets, this lag is
costly.

**No regime awareness**: The strategy trades identically in trending and
ranging markets. A more sophisticated approach would detect market regime
and adjust behavior accordingly.

**All-in sizing**: Using 100% of cash maximizes return but also maximizes
risk. A sudden adverse move can wipe out significant capital before the
stop loss triggers.

## Variations to Explore

Try modifying the example to explore different behaviors:

**Adjust window lengths**: Shorter windows (5/15) react faster but whipsaw
more. Longer windows (20/50) are smoother but lag more.

**Change position sizing**: Use 50% cash instead of 100% to reduce risk.
Compare returns and drawdowns.

**Remove risk management**: Set stop loss and take profit to NAN to rely
purely on strategy signals. Observe how this changes trade characteristics.

**Add slippage variability**: Switch to RANDOM_SLIPPAGE with stddev to
model execution uncertainty.

**Try different assets**: Run on different stocks, ETFs, or
cryptocurrencies. Crossover strategies behave differently depending on
asset volatility and trend characteristics.

## Code Location

Source: `examples/backtest/example01.c`

Strategy implementation: `src/strategies/crossover.c`
