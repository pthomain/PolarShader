# Embedded PSC playlist

Drop `.psc` scene files in `build/psc/` for local firmware builds.

PlatformIO generates an embedded playlist header before each build. When at
least one `.psc` file is present, MCU renderers play those scenes in random
order, changing every 30 seconds.
