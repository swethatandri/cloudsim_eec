machine class:
{
# Stress test: High task load with strict deadlines.
        Number of machines: 10
        CPU type: X86
        Number of cores: 4
        Memory: 8192
        S-States: [150, 130, 110, 90, 50, 20, 0]
        P-States: [15, 10, 7, 5]
        C-States: [15, 5, 2, 0]
        MIPS: [1500, 1000, 750, 500]
        GPUs: no
}
task class:
{
        Start time: 50000
        End time: 1000000
        Inter arrival: 2000
        Expected runtime: 1500000
        Memory: 6
        VM type: LINUX_RT
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: STREAM
        Seed: 104
}
