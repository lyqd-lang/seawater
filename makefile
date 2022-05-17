.PHONY: release debug windows clean

PY:=

ifeq ($(OS),Windows_NT)
	PY:=py
else
	PY:=python3
endif

release:
	$(PY) build.py
debug:
	$(PY) build.py -d
windows:
	$(PY) build.py -c i686-mingw32-gcc
clean:
	$(PY) build.py --clean
