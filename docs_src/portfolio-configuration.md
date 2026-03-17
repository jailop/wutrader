# Portfolio Configuration Guide

Understanding how to configure a Wu portfolio is central to building meaningful backtests. This guide explains the `WU_PortfolioParams` structure and the concepts behind each setting.

## Overview

Portfolio configuration controls how your strategy executes trades—execution methods, transaction costs, borrowing costs, position sizing, and risk management rules. These decisions fundamentally affect backtest results.

The `WU_PortfolioParams` structure groups related settings into logical sections:

```c
typedef struct {
    WU_Direction direction;                      // Long only, short only, or both
    double initial_cash;                         // Starting capital
    WU_ExecutionPolicy execution_policy;         // How orders execute
    WU_BorrowParams borrow_params;               // Shorting costs
    WU_PositionSizing position_sizing;           // How much to trade
} WU_PortfolioParams;
```

Each section is covered below.

## Direction: What Can You Trade?

The `direction` field determines whether positions can be long, short, or both:

```c
typedef enum {
    WU_DIRECTION_LONG_ONLY,    // Buy only, never short
    WU_DIRECTION_SHORT_ONLY,   // Sell short only, never buy
    WU_DIRECTION_BOTH          // Long and short positions allowed
} WU_Direction;
```

**Long Only** is the simplest. You buy and hold. No borrowing costs. Maximum loss is your initial investment. Suits conservative strategies or when short selling is restricted.

**Short Only** inverts the logic. You borrow and sell, banking on price declines. Less common and requires careful risk management.

**Both** allows the strategy to alternate between long and short. Our pairs trading example uses this—going long the spread when it's cheap and short when it's expensive. This requires margin and incurs borrowing costs for short legs.

## Execution Policy: How Orders Execute

This section defines the mechanics of order execution:

```c
typedef struct {
    WU_ExecutionPolicy policy;           // Execution method
    double execution_mean;               // Expected slippage or limit price
    double execution_stddev;             // Variability in execution
    WU_TransactionCostType tx_cost_type; // Fixed, proportional, or both
    double tx_cost_value;                // Cost amount or percentage
    double stop_loss_pct;                // Risk management: exit if down X%
    double take_profit_pct;              // Risk management: exit if up X%
} WU_ExecutionPolicy;
```

### Execution Methods

**Fixed Slippage** (`WU_EXECUTION_POLICY_FIXED_SLIPPAGE`):

Orders execute at the current price plus a fixed slippage amount:

```c
.policy = WU_EXECUTION_POLICY_FIXED_SLIPPAGE,
.execution_mean = 0.0005,    // 0.05% slippage
.execution_stddev = 0.0      // Deterministic
```

For a $100 stock with 0.05% slippage, you'd execute at $100.05 when buying (worst case). This is simple and predictable but unrealistic—real slippage varies with market conditions and order size.

**Market Conditions** (`WU_EXECUTION_POLICY_MARKET_CONDITIONS`):

Execution price depends on volatility:

```c
.policy = WU_EXECUTION_POLICY_MARKET_CONDITIONS,
.execution_mean = 0.0005,    // Base slippage
.execution_stddev = 0.001    // Variable component
```

Slippage increases during volatile periods. Wu models this by adding random noise to the base slippage, making it semi-realistic but still simplified.

**Next Close** (`WU_EXECUTION_POLICY_NEXT_CLOSE`):

Orders don't execute immediately. Instead, they execute at the next bar's close price. This is common in backtests that use daily data—a signal generated during the day executes at the day's close or the next day's open.

```c
.policy = WU_EXECUTION_POLICY_NEXT_CLOSE
```

This avoids look-ahead bias but introduces execution delay. The strategy might signal entry at day's close, but the actual position opens the next bar.

### Transaction Costs

Costs matter. They accumulate quickly and erode returns.

**Proportional costs** scale with trade size:

```c
.tx_cost_type = WU_TRANSACTION_COST_PROPORTIONAL,
.tx_cost_value = 0.001    // 0.1% per trade
```

Buy $10,000 of stock? Pay $10 in costs. Typical for modern retail brokers. On 100 trades, 0.1% costs eat ~10% of capital (if trades were at neutral P&L).

**Fixed costs** charge per trade:

```c
.tx_cost_type = WU_TRANSACTION_COST_FIXED,
.tx_cost_value = 5.0      // $5 per trade
```

More typical for traditional brokers. High-frequency trading often uses this model.

**Both** combines them:

```c
.tx_cost_type = WU_TRANSACTION_COST_BOTH,
.tx_cost_value = 5.0       // Fixed component
.execution_mean = 0.001    // Proportional component
```

### Risk Management Rules

**Stop Loss** (`stop_loss_pct`):

Exit a losing position if it's down X% from entry:

```c
.stop_loss_pct = 0.05      // Exit if down 5%
```

If you enter a position at $100 and it falls to $95, the position closes automatically. Set to `NAN` to disable.

**Take Profit** (`take_profit_pct`):

Exit a winning position if it's up X% from entry:

```c
.take_profit_pct = 0.10    // Exit if up 10%
```

Close winners early to lock in gains. Set to `NAN` to disable.

These are portfolio-level rules, not strategy logic. They act as safety nets. Most modern strategies don't use them—instead they rely on signals to manage positions.

## Borrow Parameters: Cost of Short Selling

Shorting requires borrowing stock. Real borrowing costs vary daily, but Wu uses a simplified model:

```c
typedef struct {
    double rate;     // Annual interest rate
    double limit;    // Maximum amount you can borrow
} WU_BorrowParams;
```

**Rate**: The annual interest charged for borrows:

```c
.borrow_params = {
    .rate = 0.05,    // 5% annual
    .limit = 100000.0
}
```

If you borrow $10,000 for a year at 5%, you pay $500 in interest. Wu prorates this daily based on how long you hold the position. Shorter positions accrue less cost.

**Limit**: Realistically, brokers won't lend unlimited capital. Set a ceiling:

```c
.limit = 100000.0  // Can't borrow more than $100k
```

Try to short $150,000? Request gets rejected or scaled down. This prevents unrealistic leverage in backtests.

## Position Sizing: How Much to Trade

This controls the quantity of each trade:

```c
typedef struct {
    WU_PositionSizeType size_type;    // Sizing method
    double size_value;                 // Parameter for the method
} WU_PositionSizing;
```

**Percentage of Cash** (`WU_POSITION_SIZE_PCT`):

Invest a fixed percentage of current cash:

```c
.position_sizing = {
    .size_type = WU_POSITION_SIZE_PCT,
    .size_value = 0.5   // 50% of cash per position
}
```

With $100,000 and 50% sizing, each trade uses $50,000. As cash fluctuates, position sizes adapt. This is the most common approach.

**Fixed Dollar Amount** (`WU_POSITION_SIZE_FIXED_AMOUNT`):

Always trade the same dollar value:

```c
.size_type = WU_POSITION_SIZE_FIXED_AMOUNT,
.size_value = 10000.0   // Always $10k per trade
```

Simple but rigid. If your account grows to $500,000, you're still trading $10k per position. Doesn't scale.

**Percentage of Equity** (`WU_POSITION_SIZE_PCT_EQUITY`):

Invest a percentage of total portfolio value:

```c
.size_type = WU_POSITION_SIZE_PCT_EQUITY,
.size_value = 0.3   // 30% of total portfolio
```

Similar to percentage of cash but includes position values. Grows as your portfolio grows, making it adaptive.

**Fixed Share Quantity** (`WU_POSITION_SIZE_FIXED_SHARES`):

Trade a fixed number of shares:

```c
.size_type = WU_POSITION_SIZE_FIXED_SHARES,
.size_value = 100.0   // Always 100 shares
```

Suitable for backtests where share count is more natural than dollar amounts. Less common.

## Practical Example

Here's a complete configuration for conservative pairs trading:

```c
WU_PortfolioParams params = {
    .direction = WU_DIRECTION_BOTH,  // Allow longs and shorts
    .initial_cash = 100000.0,        // Start with $100k
    .execution_policy = {
        .policy = WU_EXECUTION_POLICY_FIXED_SLIPPAGE,
        .execution_mean = 0.0005,    // 0.05% slippage
        .execution_stddev = 0.0,
        .tx_cost_type = WU_TRANSACTION_COST_PROPORTIONAL,
        .tx_cost_value = 0.001,      // 0.1% transaction cost
        .stop_loss_pct = NAN,        // No automatic stop loss
        .take_profit_pct = NAN       // No automatic take profit
    },
    .borrow_params = {
        .rate = 0.05,                // 5% annual borrow rate
        .limit = 100000.0            // Max $100k short
    },
    .position_sizing = {
        .size_type = WU_POSITION_SIZE_PCT,
        .size_value = 0.45           // 45% of cash per position
    }
};
```

This setup:
- Trades both long and short with modest position sizes (45% per leg)
- Accepts 0.05% slippage on execution
- Pays 0.1% in transaction fees per trade
- Borrows at 5% annually for shorts
- No automated risk stops (strategy manages exits)

## Tips for Configuration

**Conservative Approach**: Lower position sizing (20-30%), higher transaction costs, smaller borrow limits. Good for learning.

**Realistic Costs**: Use 0.05-0.1% for proportional transaction costs if you have modern retail broker pricing. Add $5-10 fixed costs if applicable.

**Slippage**: 0.05-0.2% is reasonable for liquid equities. Less liquid assets need higher values.

**Borrow Rate**: 5% is typical for major US stocks. Hard-to-borrow stocks cost more, sometimes 20%+ annually.

**Position Sizing**: 25-50% of cash is common for mean-reversion strategies. Trend-following might use lower sizing due to higher volatility.

## See Also

For API details, see the [Doxygen API Reference](https://jailop.codeberg.page/wutrader/docs/html/).

For a complete working example, see the [Tutorial](tutorial.md).
