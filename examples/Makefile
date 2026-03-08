# build make in subdirs

SUBDIRS = indicators backtest

.PHONY: all $(SUBDIRS) clean

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean: $(SUBDIRS:%=clean-%)

clean-%:
	$(MAKE) -C $* clean
