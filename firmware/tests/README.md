# tests/

Host-side unit tests. They compile `core/` and `services/` with your PC's compiler and the stubs in `stubs/`, so logic can be tested before hardware arrives.

```
make test
```

Add a `test_<module>.c` per service (codec, crypto, nav math) and a target in the `Makefile`.
