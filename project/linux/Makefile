# Makefile

.PHONY: all
all: \
	libciph_agent \
	libmemif_client \
	libdpdk_cryptodev_client \
	ciph_app \
	ciph_app_test

.PHONY: libmemif_client
libmemif_client:
	$(MAKE) -C ../../libmemif_client/project/linux -f Makefile

.PHONY: libdpdk_cryptodev_client
libdpdk_cryptodev_client:
	$(MAKE) -C ../../libdpdk_cryptodev_client/project/linux -f Makefile

.PHONY: libciph_agent
libciph_agent: libmemif_client
	$(MAKE) -C ../../libciph_agent/project/linux -f Makefile

.PHONY: ciph_app
ciph_app: libmemif_client libdpdk_cryptodev_client
	$(MAKE) -C ../../ciph_app/project/linux -f Makefile

.PHONY: ciph_app_test
ciph_app_test: libmemif_client libciph_agent
	$(MAKE) -C ../../ciph_app_test/project/linux -f Makefile

.PHONY: libmemif_client_clean
libmemif_client_clean:
	$(MAKE) -C ../../libmemif_client/project/linux -f Makefile clean

.PHONY: libdpdk_cryptodev_client_clean
libdpdk_cryptodev_client_clean:
	$(MAKE) -C ../../libdpdk_cryptodev_client/project/linux -f Makefile clean

.PHONY: libciph_agent_clean
libciph_agent_clean:
	$(MAKE) -C ../../libciph_agent/project/linux -f Makefile clean

.PHONY: ciph_app_clean
ciph_app_clean:
	$(MAKE) -C ../../ciph_app/project/linux -f Makefile clean

.PHONY: ciph_app_test_clean
ciph_app_test_clean:
	$(MAKE) -C ../../ciph_app_test/project/linux -f ciph_app_test.mak clean

.PHONY: clean
clean: libmemif_client_clean libdpdk_cryptodev_client_clean libciph_agent_clean
	$(MAKE) -C ../../ciph_app/project/linux -f Makefile clean
	$(MAKE) -C ../../ciph_app_test/project/linux -f Makefile clean

