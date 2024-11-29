machine class:
{
# Debugging edge cases: Limited resources to stress-test the scheduler.
        Number of machines: 2
        CPU type: X86
        Number of cores: 2
        Memory: 2048
        S-States: [100, 80, 60, 40, 20, 5, 0]
        P-States: [12, 8, 6, 4]
        C-States: [12, 3, 1, 0]
        MIPS: [1000, 800, 600, 400]
        GPUs: no
}
task class:
{
        Start time: 100000
        End time: 1200000
        Inter arrival: 20000
        Expected runtime: 2000000
        Memory: 1
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA2
        CPU type: X86
        Task type: CRYPTO
        Seed: 54321
}
