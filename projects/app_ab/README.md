# A/B OTA Application Project

* [中文](./README_CN.md)

## Overview

`app_ab` is the BK7258 reference project for position-independent A/B OTA updates. Its application startup follows the standard AP/CP flow, while its configuration and partition files enable A/B image packaging and update support.

Key characteristics:

- A/B OTA and HTTP OTA support
- Position-independent application images
- Hash verification for OTA images
- A secondary application area (`s_app`) and OTA finalization metadata (`ota_fina_executive`)

## Project layout

- `ap/ap_main.c`: AP application entry
- `cp/cp_main.c`: CP application entry and CP1 startup
- `ap/config/bk7258_ap/config`: AP configuration, including A/B OTA options
- `cp/config/bk7258/config`: CP configuration
- `partitions/bk7258/auto_partitions.csv`: A/B flash partition layout
- `partitions/bk7258/ab_position_independent.csv`: position-independent image setting
- `partitions/bk7258/ota_rbl.config`: OTA package settings

## Build

Run from the SDK root:

```text
make bk7258 PROJECT=app_ab
```

## Run and validation

1. Build the project and flash the generated initial images.
2. Prepare an update package using the generated A/B OTA artifact and the SDK packaging flow.
3. Start the update through the enabled OTA interface. The payload is written into `s_app`.
4. Reset the board and verify that it boots the updated image.

The partition layout and OTA package configuration are part of the update contract. Review them before changing image sizes or enabling additional features.
