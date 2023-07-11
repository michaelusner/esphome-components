CWD = $(shell pwd)
DEVICE_IP = "192.168.1.155"

.PHONY: build
build:
	docker run --rm --network host -v ${CWD}:/config esphome/esphome compile pool.yaml

.PHONY: lint
lint:
	docker run --rm -v "${PWD}/":/esphome -it ghcr.io/esphome/esphome-lint .

.PHONY: logs
logs:
	docker run --rm --network host -v ${CWD}:/config esphome/esphome logs --device $(DEVICE_IP) pool.yaml

.PHONY: flash
flash:
	docker run --rm --network host -v ${CWD}:/config esphome/esphome upload --device $(DEVICE_IP) pool.yaml

.PHONY: run
run:
	docker run --rm --network host -v ${CWD}:/config esphome/esphome run --device $(DEVICE_IP) pool.yaml

.PHONY: clean
clean:
	docker run --rm --network host -v ${CWD}:/config esphome/esphome clean pool.yaml
