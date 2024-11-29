# Machine Classes
machine class:
{
        Number of machines: 10
        CPU type: X86
        Number of cores: 8
        Memory: 16384
        S-States: [150, 130, 110, 90, 50, 20, 0]
        P-States: [15, 10, 7, 5]
        C-States: [15, 5, 2, 0]
        MIPS: [1500, 1200, 800, 500]
        GPUs: yes
}
machine class:
{
        Number of machines: 8
        CPU type: ARM
        Number of cores: 16
        Memory: 32768
        S-States: [150, 130, 110, 90, 50, 20, 0]
        P-States: [15, 10, 7, 5]
        C-States: [15, 5, 2, 0]
        MIPS: [2000, 1500, 1000, 750]
        GPUs: no
}

# Task Classes
task class:
{
        Start time: 60000
        End time : 800000
        Inter arrival: 10000
        Expected runtime: 500000
        Memory: 6
        VM type: LINUX_RT
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 12345
}
task class:
{
        Start time: 70000
        End time : 900000
        Inter arrival: 15000
        Expected runtime: 700000
        Memory: 8
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA1
        CPU type: ARM
        Task type: STREAM
        Seed: 67890
}
task class:
{
        Start time: 80000
        End time : 950000
        Inter arrival: 20000
        Expected runtime: 800000
        Memory: 10
        VM type: LINUX_RT
        GPU enabled: no
        SLA type: SLA2
        CPU type: ARM
        Task type: WEB
        Seed: 11223
}
task class:
{
        Start time: 90000
        End time : 1000000
        Inter arrival: 30000
        Expected runtime: 600000
        Memory: 4
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA1
        CPU type: X86
        Task type: WEB
        Seed: 44556
}
task class:
{
        Start time: 100000
        End time : 1100000
        Inter arrival: 25000
        Expected runtime: 900000
        Memory: 12
        VM type: LINUX_RT
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: STREAM
        Seed: 77889
}
