# Position Sizing

Position sizing determines how much to trade when signals arrive. Wu
provides four approaches, each with different characteristics.

## Position Sizing Types

### WU_POSITION_SIZE_ABS

Absolute sizing trades a fixed quantity regardless of capital or prices.
Setting `size_value = 100.0` means always trading exactly 100 shares.

**When to use**: Fixed quantity strategies, replicating specific
portfolios, or testing strategies where position size shouldn't vary.

**Limitation**: Doesn't scale with capital. If your portfolio grows from
$100k to $1M, you're still trading 100 shares. Similarly, if a stock
splits or reprices, your absolute quantity might become inappropriate.

### WU_POSITION_SIZE_PCT

Percentage sizing allocates a fixed percentage of available cash per
trade. With `size_value = 0.5`, each buy signal consumes 50% of current
cash.

**When to use**: Most single-asset strategies. The position size scales
naturally as capital grows or shrinks. If you double your money, position
sizes double automatically.

**Multi-asset behavior**: Each asset uses the specified percentage of
available cash independently. With two assets at 50% each, if both signal
buys simultaneously, you allocate 100% of cash (50% + 50%). Cash
splitting handles this (see below).

**Cash management**: "Available cash" means current cash in the account.
If you have $100k and buy $50k worth, the next buy at 50% will use $25k
(50% of remaining $50k), not $50k.

### WU_POSITION_SIZE_PCT_EQUAL

Equal allocation divides portfolio value equally across all assets.
Setting `size_value = 0.9` with three assets gives each asset 30% of
total portfolio value (90% ÷ 3).

**When to use**: Balanced portfolios, index replication, or strategies
that want equal risk across assets (assuming similar volatilities).

**Calculation**: Uses total portfolio value (cash + positions), not just
cash. This provides automatic rebalancing. If one asset grows faster than
others, its allocation naturally exceeds its share. On the next trade, it
gets allocated less to bring things back into balance.

**Multi-asset requirement**: Only makes sense with multiple assets. For
single-asset portfolios, use PCT instead.

### WU_POSITION_SIZE_STRATEGY_GUIDED

The strategy controls allocation by setting each `signal.quantity` field
to the desired portfolio percentage (0.0 to 1.0). The portfolio allocates
that percentage of total portfolio value to the trade.

**When to use**: Sophisticated allocation strategies where sizing depends
on signal strength, volatility, correlation, or other factors. Risk
parity, inverse volatility weighting, and momentum-based sizing all fit
this model.

**Implementation**: Your strategy calculates desired allocations and sets
them in signals:

```c
signals[0].quantity = 0.6;  // 60% to first asset
signals[1].quantity = 0.4;  // 40% to second asset
```

The portfolio converts these percentages to actual quantities based on
current prices and portfolio value.

## Cash Splitting

When multiple buy signals arrive simultaneously with PCT sizing, cash
splitting prevents over-allocation. Without splitting, each signal would
try to use 50% of cash, potentially allocating 100% or more total.

**How it works**: The portfolio counts buy signals, divides available
cash equally among them, and processes each signal with its cash
allocation. If you have $100k and two buy signals at 50% each, each
signal gets $50k to work with.

**When it applies**: Only for `WU_POSITION_SIZE_PCT` when multiple buy
signals arrive in the same update. Other sizing modes don't need
splitting because they have different allocation semantics.

**Strategy-guided exception**: If any signal in the batch has non-default
quantity, cash splitting is disabled. The strategy is assumed to be
managing allocations explicitly.

## Fractional Shares

Wu supports fractional shares. Position quantities are `double`, not
integers. If you have $1000 cash, 50% sizing, and a stock at $333.33, you
buy 1.500015 shares.

Most brokers now support fractional shares for many stocks, making this
reasonably realistic. For assets that don't support fractional trading
(options, futures), you'd need to implement quantity rounding in a custom
portfolio.

## Position Limits

The portfolio doesn't enforce position count limits. You can accumulate
multiple positions in the same asset (buy multiple times without selling
between). The portfolio tracks them as separate position records with
different entry prices and timestamps.

This matches real trading where you might scale into positions over time.
When selling, the portfolio closes all positions in that asset
simultaneously.

## Margin and Leverage

For long positions, you can spend cash you have. Borrowing (`borrow_params`)
only applies to short positions. The `borrow_params.limit` caps the total
dollar value of short positions, effectively limiting leverage.

There's no leverage on the long side. If you want to model margin trading
for long positions (borrowing cash to buy more), implement a custom
portfolio with that logic.

## Design Evolution

Position sizing is an area under active exploration. Current
implementation handles common cases but has rough edges:

- Cash splitting logic is heuristic
- No sophisticated risk-based sizing (Kelly criterion, etc.)
- Rebalancing is implicit, not explicit
- No transaction cost awareness in sizing decisions

These limitations are acceptable for a learning project but would need
refinement for serious backtesting.
