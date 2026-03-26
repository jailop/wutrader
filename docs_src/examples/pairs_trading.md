# Pairs Trading Example

This example demonstrates a pairs trading strategy on two correlated
assets (SPY and QQQ). The strategy bets on mean reversion when the spread
between the two assets deviates from its historical average.

## The Strategy

Pairs trading exploits temporary divergences between correlated assets.
When two stocks historically move together but temporarily drift apart,
the strategy bets they'll converge back. Go long the underperformer,
short the outperformer, and profit when they revert to typical
relationship.

The implementation calculates the spread as `spread = asset1 - asset2`.
Over a rolling window, it tracks the spread's mean and standard
deviation. When the spread exceeds `mean + threshold × stdev`, go short
asset 1 and long asset 2. When it drops below `mean - threshold × stdev`,
reverse positions. When the spread returns to its mean, close both
positions.

## Code Structure

The example in `examples/backtest/pairs_trading.c`:

```c
// Open CSV files for both assets
FILE* spy_file = fopen(argv[1], "r");
FILE* qqq_file = fopen(argv[2], "r");

WU_Reader spy_reader = wu_csv_reader_new(
    spy_file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_SECONDS,
    true
);
WU_Reader qqq_reader = wu_csv_reader_new(
    qqq_file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_SECONDS,
    true
);

// Create pairs trading strategy
// window=20, threshold=2.0, hedge_ratio=1.0
WU_Strategy strategy = wu_pairs_trading_strat_new(20, 2.0, 1.0);

// Configure multi-asset portfolio
WU_PortfolioParams params = {
    .direction = WU_DIRECTION_BOTH,
    .initial_cash = 100000.0,
    .execution_policy = {
        .policy = WU_EXECUTION_POLICY_FIXED_SLIPPAGE,
        .execution_mean = 0.0005,
        .execution_stdev = 0.0,
        .tx_cost_type = WU_TRANSACTION_COST_PROPORTIONAL,
        .tx_cost_value = 0.001,
        .stop_loss_pct = NAN,
        .take_profit_pct = NAN
    },
    .borrow_params = {
        .rate = 0.05,
        .limit = 100000.0
    },
    .position_sizing = {
        .size_type = WU_POSITION_SIZE_PCT,
        .size_value = 0.45
    }
};

WU_BasicPortfolio portfolio = wu_basic_portfolio_new(
    params,
    wu_symbol_list("SPY", "QQQ")
);

// Run backtest
WU_Runner runner = wu_runner_new(
    WU_PORTFOLIO(portfolio),
    WU_STRATEGY(strategy),
    wu_reader_list(WU_READER(spy_reader), WU_READER(qqq_reader))
);

wu_runner_exec(runner, false);
```

## Configuration Details

**Strategy parameters**: The 20-period window determines how much history
to use for spread statistics. Shorter windows react faster to changes but
are noisier. The 2.0 threshold means we enter when the spread is 2
standard deviations from its mean—approximately the 95th percentile for
normally distributed spreads. The 1.0 hedge ratio means we trade equal
dollar amounts of both assets.

**Direction**: Must be `WU_DIRECTION_BOTH` since pairs trading requires
both long and short positions simultaneously. We're long one asset while
shorting the other.

**Position sizing**: At 45% per asset, we can allocate up to 90% of cash
across both positions. The 10% buffer handles small cash fluctuations
from price movements and costs.

**Borrowing**: The 5% annual rate models the cost of borrowing shares to
short. The $100k limit caps our short exposure—we can't short more than
$100k worth of stock. This prevents excessive leverage.

**Risk management**: Stop loss and take profit are disabled (NAN). Pairs
trading is a mean-reversion strategy that expects temporary drawdowns
before convergence. Automatic exits would interfere with the strategy
logic.

## Multi-Asset Portfolio Behavior

The portfolio manages both assets with a shared cash pool. When the
strategy says "buy SPY, sell QQQ", the portfolio:

1. Executes the SPY buy, deducting cash
2. Executes the QQQ short, adding cash from the sale
3. Tracks both positions independently
4. Pays borrowing interest on the QQQ short over time

Position quantities are tracked separately but cash is fungible. Closing
the SPY position returns cash that could be used for either asset.

## Running the Example

```bash
# Build
make

# Run on test data
./examples/backtest/pairs_trading ./tests/data/spy.csv ./tests/data/qqq.csv

# Verbose mode shows each trade
./examples/backtest/pairs_trading ./tests/data/spy.csv ./tests/data/qqq.csv -v
```

Verbose output shows:

```
[BUY SPY] HOLD QQQ | T: 1234567890 | Cash: 95000.00
[SELL SPY] [BUY QQQ] | T: 1234567900 | Cash: 98500.00
```

## Interpreting Results

The output includes:

- Portfolio value and PnL (total and percentage)
- Transaction fees and borrowing interest (usually significant for pairs trading)
- Trade statistics (total, wins, losses, win rate)
- Current positions (quantity, value, price for each asset)

Pairs trading typically generates many trades since positions open and
close as the spread oscillates. Transaction costs accumulate
significantly. In the test data, costs often exceed $40k on $100k initial
capital.

## Known Issues

**Correlation assumption**: The strategy assumes SPY and QQQ remain
correlated. During market stress, correlations break down. The strategy
continues trading as if they'll revert, potentially accumulating losses.

**No cointegration check**: The strategy doesn't test whether the assets
are cointegrated (mean-reverting spread). It assumes they are and trades
accordingly. Real pairs trading would test cointegration first.

**Fixed hedge ratio**: The 1.0 hedge ratio means we trade equal dollar
amounts. The optimal ratio depends on the assets' price relationship and
volatilities. A more sophisticated approach would estimate the hedge
ratio dynamically.

**Capital tie-up**: Pairs positions often last many bars, tying up
capital while waiting for convergence. During this time, opportunity cost
accumulates. The strategy doesn't account for alternative uses of
capital.

## Variations to Explore

**Adjust threshold**: Lower thresholds (1.5) generate more trades but
enter on smaller divergences. Higher thresholds (2.5) generate fewer but
potentially more reliable trades.

**Change window length**: Shorter windows (10) adapt faster to changing
spread dynamics. Longer windows (30) are more stable but slower to react.

**Modify position sizing**: Try 30% per asset instead of 45% to reduce
exposure and see how risk/return changes.

**Different asset pairs**: Try other correlated pairs (XLE/XLF,
GLD/SLV). Not all pairs exhibit stable mean reversion.

**Add NEXT_CLOSE**: Change execution policy to NEXT_CLOSE to add a
one-bar delay between signal and execution. This models real-world latency
more realistically.

## Limitations

This is an educational example, not a trading system. Real pairs trading
requires:

- Statistical testing for cointegration
- Dynamic hedge ratio estimation  
- Regime detection (don't trade when correlations are breaking)
- Position sizing based on spread volatility
- Explicit entry and exit rules beyond simple threshold crossing

Use this example to understand the mechanics of multi-asset strategies
and portfolio management. Don't use it for actual trading without
substantial enhancements.

## Code Location

Example: `examples/backtest/pairs_trading.c`

Strategy implementation: `src/strategies/pairs_trading.c`
