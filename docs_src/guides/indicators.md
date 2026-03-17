# Indicators interface

This document explains the common interface used by indicators in the library and provides a brief example implementing the Simple Moving Average (SMA) indicator.

## Rolling approach

Indicators are designed to work with streaming data, meaning they update one data point at a time. When new data is pushed, the indicator is updated and older values are forgotten.

For example, computing a moving average at each update, it doesn't
include all values involved. Instead, it just takes the previous average
to update it with the proportion of the arriving value, performing a
minimal number of operations. 

$$\text{MA}_{t,p} = \text{MA}_{t-1,p} + \frac{x_t - x_{t-p}}{p}$$

Where:

* $p$: Number of time points over the moving average is computed
* $\text{MA}_{t,p}$: Moving average for time $t$
* $\text{MA}_{t-1,p}$: Previous computed moving average
* $x_t$: New arriving value
* $x_{t - p}$: Value of $p$-points before, to be forgotten


## Interface overview

All indicators should adhere to the same structure defined in the header `include/wu/indicators.h`:

- An `update` function pointer that accepts a pointer to the indicator (`self`) and a value (type varies by indicator) and returns the updated indicator value (type also varies by indicator).
- A `delete` function pointer that frees any allocated resources associated with the indicator.
- A `value` field that stores the current value of the indicator. Its type is specific to the indicator (typically `double`).

When composing an indicator value, a structure should be used. For example, the `WU_MACD` indicator employs a structure `WU_MACDResult` with the following fields: `macd`, `signal`, and `histogram`.

Convenience macros are provided for uniform usage:

- `wu_indicator_update(indicator, value)` — calls the indicator's `update` function and returns the result.
- `wu_indicator_get(indicator)` — returns the `value` field of the indicator.
- `wu_indicator_delete(indicator)` — calls the indicator's `delete` function to free resources.

## Minimal SMA example

The following shows the typical pieces required to implement an indicator.

First, define the structure to represent the indicator. This should include fields for the `update` and `delete` functions, as well as a `value` field to represent the current state of the indicator.

```c
typedef struct WU_SMA_ {
     double (*update)(struct WU_SMA_ *self, double value);
     void (*delete)(struct WU_SMA_ *self);
     double value;
     double* prev_values;
     int window_size;
     int pos;
     int len;
     double sum;
}* WU_SMA;
```

Here is the implementation of the `update` function below:


```c
static double wu_sma_update(struct WU_SMA_ *self, double value) {
    if (self->len < self->window_size) {
        self->len++;
    } else {
        // subtract overwritten value from sum
        self->sum -= self->prev_values[self->pos];
    }

    self->prev_values[self->pos] = value;
    self->sum += value;
    self->pos = (self->pos + 1) % self->window_size;

    self->value = self->sum / (double)self->len;
    return self->value;
}
```

Now, let's move on to implementing the `delete` function:

```c
static void wu_sma_delete(struct WU_SMA_ *self) {
    if (!self) return;
    free(self->prev_values);
    free(self);
}
```

Finally, here's the implementation of the `_new` function:

```c
WU_SMA wu_sma_new(int window_size) {
    WU_SMA s = malloc(sizeof(struct WU_SMA_));
    s->update = (double (*)(struct WU_SMA_*, double))wu_sma_update;
    s->delete = (void (*)(struct WU_SMA_ *))wu_sma_delete;
    s->value = 0.0;
    s->prev_values = calloc(window_size, sizeof(double));
    s->window_size = window_size;
    s->pos = 0;
    s->len = 0;
    s->sum = 0.0;
    return s;
}
```

Once the indicator has been implemented, here’s an example of how it can be used:

```c
WU_SMA sma = wu_sma_new(5);
double new_val = wu_indicator_update(sma, price);
double cur = wu_indicator_get(sma);
wu_indicator_delete(sma);
```

If more examples are desired, consider copying one of the concrete implementations from the library and adapting it for a new indicator: replicate the struct layout, implement `update` and `delete`, provide a `_new` constructor, and expose it via the existing macros for consistent usage.