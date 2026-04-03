# Performance Metrics: Risk-Adjusted Returns

Understanding raw returns tells only part of a strategy's story. A strategy that earns 20% annually with catastrophic 50% drawdowns differs fundamentally from one that earns 15% with minimal volatility. Risk-adjusted metrics like Sharpe, Sortino, and Calmar ratios help you evaluate returns in context of the risks taken to achieve them.

This guide explains Wu's three key performance metrics: **Sharpe Ratio**, **Sortino Ratio**, and **Calmar Ratio**. Each measures the relationship between returns and risk, but they define and penalize risk differently.

---

## Why Risk-Adjusted Metrics Matter

Consider two strategies over one year:

**Strategy A**: 20% return, but with wild swings—up 40% one month, down 30% the next. Maximum drawdown: 35%.

**Strategy B**: 15% return, but smooth, consistent monthly gains of ~1.2%. Maximum drawdown: 5%.

Raw returns favor Strategy A. But risk-adjusted analysis might favor Strategy B. If you had to withdraw funds during that 35% drawdown, Strategy A could lock in catastrophic losses. Strategy B's smoothness allows predictable withdrawals and compound growth.

Risk-adjusted metrics quantify this tradeoff, enabling comparison across strategies with different return and risk profiles.

---

## Sharpe Ratio: Return per Unit of Total Risk

The Sharpe Ratio measures excess return (over the risk-free rate) per unit of total volatility. It is the most widely-used risk-adjusted performance metric.

### The Formula

$$
\text{Sharpe Ratio} = \frac{R_p - R_f}{\sigma_p}
$$

Where:

* $R_p$: Average portfolio return (annualized)
* $R_f$: Risk-free rate (annualized)
* $\sigma_p$: Standard deviation of portfolio returns (annualized)
* $R_p - R_f$: Excess return over risk-free rate

Higher Sharpe ratios indicate better risk-adjusted performance. A Sharpe of 1.0 means you are earning 1 unit of excess return for each unit of volatility. Above 1.0 is generally considered good; above 2.0 is exceptional.

### Using the Sharpe Ratio

```c
#include "wu.h"

// Create a Sharpe ratio tracker
// Parameters: initial_portfolio_value, annual_risk_free_rate
WU_SharpeRatio sharpe = wu_sharpe_ratio_new(100000.0, 0.05);  // 5% risk-free rate

// Update with each portfolio value change
WU_PerformanceUpdate perf = {
    .portfolio_value = current_value,  // Current portfolio value
    .timestamp = {
        .mark = current_timestamp,    // Unix timestamp
        .units = WU_TIME_UNIT_SECONDS // Time unit of your data
    }
};

// Update the ratio (returns current Sharpe value)
double current_sharpe = wu_indicator_update(sharpe, perf);

// Or retrieve current value without updating
double sharpe_value = wu_indicator_get(sharpe);

// Clean up when done
wu_indicator_delete(sharpe);
```

The Sharpe ratio updates incrementally as new portfolio values arrive. It calculates period-to-period simple returns, maintains running statistics (mean and standard deviation), and annualizes based on the observed data frequency.

**Important**: The Sharpe ratio requires at least **30 observations** before returning a valid value. Below this threshold, it returns `NAN` to avoid statistically unreliable calculations.

### What Sharpe Penalizes

The Sharpe ratio penalizes **all volatility equally**—both upside and downside. Large positive spikes hurt your Sharpe just as much as large negative ones. This can seem counterintuitive: why penalize positive surprises?

The rationale is that volatility represents uncertainty. Even positive volatility makes the strategy harder to stick with and suggests the model does not fully capture the underlying dynamics. However, this symmetry creates tension for strategies with positively-skewed returns (many small losses, few large gains).

---

## Sortino Ratio: Return per Unit of Downside Risk

The Sortino Ratio addresses Sharpe's symmetry problem. It only penalizes downside volatility—the returns that actually hurt your portfolio.

### The Formula

$$
\text{Sortino Ratio} = \frac{R_p - R_f}{\sigma_d}
$$

Where:

* $\sigma_d$: **Downside deviation** (standard deviation of returns below target)

The denominator changes from total standard deviation to downside deviation, calculated as:

$$
\sigma_d = \sqrt{\frac{1}{n} \sum_{r_i < \text{target}} (r_i - \text{target})^2}
$$

Only returns below the target (typically the risk-free rate or zero) count. Returns above target are ignored—they do not hurt your Sortino.

### Using the Sortino Ratio

```c
#include "wu.h"

// Create a Sortino ratio tracker
// Parameters: initial_portfolio_value, annual_risk_free_rate
WU_SortinoRatio sortino = wu_sortino_ratio_new(100000.0, 0.05);

// Update with portfolio values (same interface as Sharpe)
WU_PerformanceUpdate perf = {
    .portfolio_value = current_value,
    .timestamp = {
        .mark = current_timestamp,
        .units = WU_TIME_UNIT_SECONDS
    }
};

double current_sortino = wu_indicator_update(sortino, perf);

// Clean up
wu_indicator_delete(sortino);
```

The API mirrors the Sharpe ratio exactly. The difference is internal: Sortino tracks downside deviation separately, counting only returns below the risk-free rate threshold.

**Important**: Like Sharpe, Sortino requires **30 observations** for valid calculation.

### When Sortino Excels

Use Sortino when your strategy has **asymmetric returns**:

* **Trend following**: Many small losses, few large winners
* **Option selling**: Consistent small gains, occasional large losses
* **High-frequency trading**: Many small trades with positive expectancy

These strategies often look poor on Sharpe (high total volatility) but strong on Sortino (downside is contained).

Consider a strategy with monthly returns: +1%, +2%, +1%, -8%, +3%. The -8% month dominates the Sharpe calculation. But if your model predicts exactly this distribution—small consistent gains with occasional stops—Sortino better captures the risk you are actually taking.

---

## Calmar Ratio: Return per Unit of Maximum Pain

The Calmar Ratio takes a different approach entirely. Instead of volatility, it uses **maximum drawdown**—the worst peak-to-trough decline—as the risk measure.

### The Formula

$$
\text{Calmar Ratio} = \frac{R_p}{|\text{MDD}|}
$$

Where:

* $R_p$: Annualized portfolio return
* $\text{MDD}$: Maximum drawdown (most negative value from peak to trough)

Unlike Sharpe and Sortino, Calmar does not assume normal distributions or statistical properties. It asks a simple question: how much annual return do you generate per unit of worst-case historical loss?

### Using the Calmar Ratio

```c
#include "wu.h"

// Create a Calmar ratio tracker
// Parameter: initial_portfolio_value
WU_CalmarRatio calmar = wu_calmar_ratio_new(100000.0);

// Update with portfolio values
WU_PerformanceUpdate perf = {
    .portfolio_value = current_value,
    .timestamp = {
        .mark = current_timestamp,
        .units = WU_TIME_UNIT_SECONDS
    }
};

double current_calmar = wu_indicator_update(calmar, perf);

// Clean up
wu_indicator_delete(calmar);
```

Calmar requires **30 observations** before calculating, like the other metrics. It internally tracks maximum drawdown using the `WU_MaxDrawdown` indicator, which monitors the peak-to-trough decline from the highest portfolio value seen.

### Why Calmar Matters

Maximum drawdown represents **real pain**—the loss you would experience if you invested at the worst possible time. Unlike volatility, which is abstract, drawdown is concrete: "If I had invested at the peak, I would be down X%."

Calmar excels for:

* **Tail-risk-sensitive strategies**: Where avoiding catastrophic losses matters more than smooth returns
* **Long-term investors**: Who must endure drawdowns without panic-selling
* **Strategies with non-normal distributions**: Where standard deviation poorly captures risk

A Calmar of 2.0 means you have historically earned 2% annually for every 1% of maximum drawdown. Higher is better; above 1.0 suggests the strategy compensates adequately for its worst-case risk.

---

## Comparing the Three Metrics

| Metric | Risk Measure | Penalizes | Best For |
|--------|--------------|-----------|----------|
| **Sharpe** | Total volatility ($\sigma$) | All large deviations | Comparing strategies with similar return distributions |
| **Sortino** | Downside deviation ($\sigma_d$) | Only returns below target | Asymmetric returns, trend following |
| **Calmar** | Maximum drawdown | Worst peak-to-trough decline | Avoiding catastrophic losses, long-term investing |

### Practical Example

Consider a strategy with these monthly returns:

| Month | Return | Cumulative |
|-------|--------|------------|
| 1 | +2% | +2% |
| 2 | +3% | +5% |
| 3 | -15% | -10% |
| 4 | +4% | -6% |
| 5 | +5% | -1% |
| 6 | +3% | +2% |

**Sharpe**: Penalizes that -15% month heavily. Even though the strategy recovers, the volatility appears high.

**Sortino**: Penalizes only the negative returns (-15%, and months 3-4 while cumulative is negative). The strong recovery in months 5-6 does not count against it.

**Calmar**: Focuses on the 10% drawdown from peak (month 2) to trough (month 4). The 6-month annualized return of ~4% gives a Calmar of 0.4—not great, but captures that the strategy recovered.

### Correlation Between Metrics

In practice, Sharpe and Sortino often correlate highly (>0.9) for strategies with symmetric returns. They diverge when strategies show skewness or kurtosis.

Calmar can diverge significantly. A strategy might have good Sharpe/Sortino (controlled volatility) but terrible Calmar (one catastrophic event). This is why many practitioners use Calmar alongside one of the volatility-based metrics.

---

## Annualization: How Wu Scales Metrics

All three metrics are annualized for comparability. A Sharpe of 1.0 on daily data means something different from 1.0 on monthly data without annualization.

### The Annualization Process

Wu calculates metrics using this process:

1. **Compute period-to-period returns**: $r_t = \frac{P_t - P_{t-1}}{P_{t-1}}$

2. **Track time span**: $\Delta t = t_n - t_1$ (in time units)

3. **Calculate periods per year**:
   $$
   \text{periods per year} = \frac{\text{annualization factor} \times n}{\Delta t}
   $$

4. **Annualize volatility** (square-root-of-time rule):
   $$
   \sigma_{\text{annual}} = \sigma_{\text{period}} \times \sqrt{\text{periods per year}}
   $$

5. **Annualize return** (simple annualization):
   $$
   R_{\text{annual}} = R_{\text{total}} \times \text{periods per year}
   $$

The annualization factor depends on your time unit:

```c
// From wu/timeutils.h
double wu_annualization_factor(WU_TimeUnit unit);

// WU_TIME_UNIT_SECONDS: ~31,557,600 (seconds per calendar year)
// WU_TIME_UNIT_MILLIS: ~3.15576e9 (milliseconds per year)
// etc.
```

**Important**: The annualization assumes your data spans the entire period without gaps. For trading data, market holidays and weekends create gaps. Wu uses calendar time, not trading time. This means:

* Daily data with 252 trading days gets annualized as ~365 calendar days
* This produces slightly different results than trading-day annualization

If you need precise trading-day annualization, pre-process your data or adjust the time units accordingly.

---

## Reading the Metrics: A Practical Guide

### Portfolio Stats Integration

When running backtests, Wu automatically calculates these metrics and includes them in portfolio statistics:

```c
// After backtest completes, access stats
WU_PortfolioStats stats = portfolio->stats;

// Access metrics directly
double sharpe = wu_indicator_get(stats->sharpe_ratio);
double sortino = wu_indicator_get(stats->sortino_ratio);
double calmar = wu_indicator_get(stats->calmar_ratio);
double mdd = wu_indicator_get(stats->max_drawdown);

// Or output as JSON
char* json = wu_portfolio_stats_to_json(stats, true);
printf("%s\n", json);
free(json);
```

### Interpreting Values

**Sharpe Ratio Guidelines**:

* `< 0`: Worse than risk-free rate after volatility adjustment
* `0 - 0.5`: Poor—returns barely compensate for risk
* `0.5 - 1.0`: Acceptable—modest risk-adjusted returns
* `1.0 - 2.0`: Good—solid risk-adjusted performance
* `> 2.0`: Excellent—rare in practice

**Sortino Ratio**: Typically 20-50% higher than Sharpe for strategies with positive skew. Similar interpretation thresholds apply.

**Calmar Ratio Guidelines**:

* `< 0.5`: Poor—insufficient return for the drawdown risk
* `0.5 - 1.0`: Moderate—marginal compensation for max pain
* `1.0 - 2.0`: Good—adequate return for worst-case scenario
* `> 2.0`: Excellent—strong drawdown-adjusted returns

**Maximum Drawdown Guidelines**:

* `< -0.10` (10%): Conservative
* `-0.10` to `-0.20` (10-20%): Moderate
* `-0.20` to `-0.30` (20-30%): Aggressive
* `> -0.30` (30%+): Very aggressive

### Warning Signs

Watch for these red flags:

**Sharpe >> Sortino**: Strategy has significant downside volatility. The volatility-based Sharpe looks good, but the Sortino reveals asymmetry.

**Calmar << Sharpe**: Strategy has smooth returns but catastrophic tail risk. One bad event dominates the maximum drawdown.

**All metrics declining over time**: Strategy performance is degrading. Early success is not being replicated.

**Extremely high Sharpe (>3)**: Likely overfitting, data issues, or look-ahead bias. Verify carefully.

---

## Implementation Details

### Statistical Considerations

**Minimum Observations**: All three metrics require 30 observations before calculating. Below this threshold, `NAN` is returned. This ensures statistical reliability—the standard deviation and mean of small samples are noisy.

**Sample vs Population**: Wu uses **sample standard deviation** (degrees of freedom = 1) for Sharpe and Sortino calculations. This is standard practice for statistical inference.

**Zero Return Handling**: If all returns are identical (zero volatility), the ratio returns `NAN` to avoid division by zero. This is correct—no ratio can be calculated with no variation.

**Risk-Free Rate**: The annualized risk-free rate you provide (e.g., 0.05 for 5%) is converted to a per-period rate internally. Ensure your input is annualized, not per-period.

### Performance Characteristics

All three indicators update in **O(1)** time per observation—constant time regardless of history length. They maintain running statistics:

* **Sharpe**: Tracks running mean and variance (Welford's algorithm)
* **Sortino**: Tracks running mean and sum of squared negative returns
* **Calmar**: Delegates to `WU_MaxDrawdown`, which tracks current peak

Memory usage is **O(1)**—fixed size regardless of data stream length.

### Thread Safety

The indicators are **not thread-safe**. Each indicator instance should be accessed by only one thread at a time. If you need concurrent access, create separate instances or add external synchronization.
