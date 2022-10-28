# BeeNrf ![BeeNrf logo](graphics/bee.svg)

## How to get started with EFM8B Busy Bee Ubuntu 22.04

### 1. Install stable wine.
Install wine according to winehq: https://wiki.winehq.org/Ubuntu
Don't follow normal ubuntu guidelines, since it requires manually installing `wine32`
which wants to break half of my system.

### 2. Launch
cd $SIMPLICITY_INSTALLATION && WINEPATH=$SIMPLICITY_INSTALLATION ./studio


## BOM
| Geared 3V motor x2 | 32kr * 2 |
| Front wheel |  |
| nrf24l01 | 18kr | 
| Busy Bee 8-bit | 10kr |
| Mosfet | 2 |
| Battery charg | |

## PCB Layers
1. RF signals
2. Ground  <- Don't break!
3. Power   <- Don't break!
4. Other signals
