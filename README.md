# silly-benchmarks
In this repository I collect any standalone benchmarks that I create. Beware: Some or most of them might be rather silly, purely academic and irrelevant in practice :)

Every benchmark resides in its own sub-directory (with common code in `common/`) and comes with both a `config_template.sh` and a `build.sh` file. Simply enter the benchmark directory, `cp config_template.sh config.sh` (edit `config.sh` if you want) and `./build.sh` to build. Then `./exe` to execute the benchmark. Have fun!
