# Marina: Realizing ML-driven Real-time Network Traffic Monitoring at Terabit Scale

This repository contains material accompanying the paper **Marina: Realizing ML-driven Real-time Network Traffic Monitoring at Terabit Scale** by Michael Seufert, Katharina Dietz, Nikolas Wehner, Stefan Geißler, Joshua Schüler, Manuel Wolz, Andreas Hotho, Pedro Casas, Tobias Hoßfeld, and Anja Feldmann.

Contained are the implementations of the P4 program designed for the Tofino 1 platform as well as the data plane controller.

## Prerequisites
- bf-sde version 8.9
- A modified bf-sde is required to load the generated .so library. For copyright reasons, this is not included.
  (have a look at [Mantis: Reactive Programmable Switches](https://doi.org/10.1145/3387514.3405870), how to achieve this)

## Concrete Build and Run instructions

# General
This guide assumes you have a separate dev machine and the tofino switch and bf-sde correctly set up on the tofino switch. This repository must be on your dev machine and reverse mounted on your tofino switch (This is described in the Prerequisites section)

## Prerequisites

1. Setup ssh key authentication for tofino switch
<!-- e.g. https://www.digitalocean.com/community/tutorials/how-to-configure-ssh-key-based-authentication-on-a-linux-server-de -->
    
2. Create easy to access hostname for ssh so you can connect with `ssh shortcut_name`

        (in ~/.ssh/config)
        Host shortcut_name
            HostName host_ip
            User your_username
            ServerAliveInterval 30

3. Connect to tofino with `ssh shortcut_name -R 10000:localhost:22`
   
4. Start sshd service on your dev machine (open port if necessary)
   
5. Mount with `sshfs -p 10000 -o idmap=user,nonempty username@127.0.0.1:/home/username/p4_Controller server_folder` on server
   (clear .ssh/known_hosts if an entry for localhost already exists on the server)
   
6. Later unmount with `fusermount -uz server_folder` on server
   
7. `.profile` has to contain

        . /home/bf-sde-8.9.2/sde/set_sde.bash
        export BSP=/home/username
        export BSP_INSTALL=/home/username/bf-sde-8.9.2/install

8. Make sure a symlink to your sde version exists `ln -s bf-sde-8.9.2/ sde`

9.  Create Python venv in p4_Controller `python3 -venv p4_Controller/` (dev machine)

10. `source bin/activate`

11. `pip install -r requirements.txt`

## Configuration

Adjust settings in `controller/config.h` and `p4/config.p4`.

## Building P4
Take a look into `build.sh` to make sure all paths are correct

- Generate autogenerated files (create both autogenerated folders before) (best on dev machine, because you need a current python version)
  
        python3 create_static_tables.py p4/autogenerated/table_sizes.p4 controller/autogenerated/static_tables.h

- Run ./build.sh on dev machine

## Building Controller

You can build a release or a debug build, adjust the commands accordingly. (The following has to be done on the tofino switch)

1. Create folder `cmake-build-debug` if it does not exist
   
2. In the folder: `cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" ..` (or `DCMAKE_BUILD_TYPE=Release`)
   
3. In the folder: `make -j8 controller && cp libcontroller.so ~/sde/ma_lowlevellib.so`  
  (`make -j8 benchmark_name` to build other benchmarks, some are standalone some also are .so files)

## Running
Make sure you are in ~/sde (this needs to be your working dir, to load the controller)

- `./run_switchd.sh -p marina_data_plane.p4`

