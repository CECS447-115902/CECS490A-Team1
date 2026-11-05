# tests/

Host-side unit tests. They compile `services/` with your PC's compiler, with no HAL and no FreeRTOS, so logic can be tested before hardware arrives.

Planned; no tests yet. They will run with `make test` once services exist.

Keep services free of FreeRTOS and HAL calls so they stay testable here: the task bodies call into services, not the other way round. Add a `test_<module>.c` per service (packet codec, crypto, nav math) and a target in the `Makefile`.
