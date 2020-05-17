TOMASULO := ../build/tomasulo
SIMULATOR := ../build/simulator
PREFIX := 

all: $(patsubst %.nel,%.log,$(wildcard *.nel))

diff: $(patsubst %.nel,%.diff,$(wildcard *.nel))

log: $(patsubst %.nel,../Log/$(PREFIX)_%.log,$(wildcard *.nel))

../Log/$(PREFIX)_%.log: %.log
	cp $^ $@

%.log: %.nel $(TOMASULO)
	$(TOMASULO) $< $(patsubst %.nel,%.log,$<)

%.reg: %.nel $(TOMASULO)
	$(TOMASULO) $< $(patsubst %.nel,%.log,$<) $(patsubst %.nel,%.trace,$<) $(patsubst %.nel,%.reg,$<)

%.sim_reg %.sim_trace: %.reg $(SIMULATOR)
	$(SIMULATOR) $(patsubst %.reg,%.nel,$<) $(patsubst %.reg,%.sim_trace,$<) $@

%.diff: %.sim_reg
	diff -u $^ $(patsubst %.sim_reg,%.reg,$<) > $@

clean:
	rm -f *.log *.trace *.reg *.sim_reg *.sim_trace *.diff
