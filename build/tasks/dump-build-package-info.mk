stamp := $(PRODUCT_OUT)/dump-building-packages.info

clean_stamp := $(shell rm $(stamp))

print-build-packages := $(foreach m, $(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES), \
    $(shell echo -e "$(m)\t$(ALL_MODULES.$(m).CLASS)\t$(ALL_MODULES.$(m).PATH)" >> $(stamp)))

$(stamp) : $(print-build-packages)
	$(clean_stamp)
	@echo "Result will print info $(stamp)"
	$(print-build-packages)

.PHONY: dump-building-packages
dump-building-packages : $(stamp)

#####

stamp := dump-all-packages.info

clean_stamp := $(shell rm $(stamp))

print-all-packages := $(foreach m, $(ALL_MODULES), \
    $(shell echo -e "$(m)\t$(ALL_MODULES.$(m).CLASS)\t$(ALL_MODULES.$(m).PATH)" >> $(stamp)))

$(stamp) : $(print-all-packages)
	@echo "Result will print to $(stamp)"
	$(clean_stamp)
	$(print-all-packages)

.PHONY: dump-all-packages
dump-all-packages : $(stamp)
