# PBAP PCE Demo

* [中文](./README_CN.md)

## Function
This demo is used for PBAP PCE API functional verification.
It provides CLI commands to initialize PBAP PCE, connect/disconnect a remote device,
handle OBEX authentication, browse phonebook folders, get phonebook size, download
phonebook data, list vCard entries, get one vCard, and abort an ongoing operation.

Source files:
- pbap_pce_demo.c: Implements PBAP PCE callback registration, init/deinit, event queue/thread, and PBAP event handling.
- pbap_pce_demo_cli.c: Implements the "pbap" CLI command and command parameter parsing.

CLI usage:
If running on the SMP branch, add the "ap_cmd" prefix before the command, for example:

```text
ap_cmd pbap init
pbap init
pbap deinit
pbap connect xx:xx:xx:xx:xx:xx [auth [pin [user_id]]]
pbap disconnect
pbap auth <pin> [user_id]
pbap get_phonebook pb|mch|och|ich|cch|spd|fav [offset]
pbap set_path root|parent|child [folder_name]
pbap get_vcard_list [folder_name] [offset]
pbap get_vcard [vcard_name]
pbap get_size pb|mch|och|ich|cch|spd|fav
pbap abort
```

Command description:
- pbap init: Create PBAP event queue/thread, register PBAP callback, and call bk_bt_pbap_pce_init().
- pbap deinit: Call bk_bt_pbap_pce_deinit(), unregister callback, and stop the PBAP event task.
- pbap connect xx:xx:xx:xx:xx:xx [auth [pin [user_id]]]: Connect to the remote PBAP server. If "auth" is present, optional pin and user_id are also passed during connect. The connected device address is saved and reused by following commands.
- pbap disconnect: Disconnect from the saved connected device.
- pbap auth <pin> [user_id]: Send OBEX authentication response for the saved connected device. When the peer actively initiates authentication, use this command to reply with the response.
- pbap get_phonebook pb|mch|och|ich|cch|spd|fav [offset]: Download one phonebook object or one call history object from the remote device. Offset is optional and defaults to 0.
- pbap set_path root|parent|child [folder_name]: Change the current PBAP path on the remote device.
- pbap get_vcard_list [folder_name] [offset]: Get the vCard listing of the current folder or of the given folder name. If folder_name is omitted, the default object name is "pb". Offset is optional and defaults to 0.
- pbap get_vcard [vcard_name]: Download one vCard object. If vcard_name is omitted, the default object name is "0.vcf". The vcard_name should be selected from the handle returned by pbap get_vcard_list, for example "0.vcf".
- pbap get_size pb|mch|och|ich|cch|spd|fav: Get remote phonebook size for the specified object.
- pbap abort: Abort the current PBAP operation for the saved connected device.

Object mapping:

- pb  -> main phonebook, telecom/pb.vcf
- mch -> missed call history, telecom/mch.vcf
- och -> outgoing call history, telecom/och.vcf
- ich -> incoming call history, telecom/ich.vcf
- cch -> combined call history, telecom/cch.vcf
- spd -> speed dial, telecom/spd.vcf
- fav -> favorite contacts, telecom/fav.vcf

Set path examples:

- pbap set_path root
- pbap set_path child telecom
- pbap set_path child pb
- pbap set_path child cch

PBAP virtual folders structure:

```text
root
|-- telecom
|   |-- pb
|   |-- ich
|   |-- och
|   |-- mch
|   |-- cch
|   |-- spd
|   |-- fav
|-- SIM1
    |-- telecom
        |-- pb
        |-- ich
        |-- och
        |-- mch
        |-- cch
```

Typical test steps:
- Initialize PBAP PCE:

```text
pbap init
```

- Connect remote device:

```text
pbap connect xx:xx:xx:xx:xx:xx
```

- If remote side requires OBEX authentication:

```text
pbap auth 123456
```

- Get main phonebook:

```text
pbap get_phonebook pb 0
```

- Get missed call history:

```text
pbap get_phonebook mch 0
```

- Browse folders:

```text
pbap set_path root
pbap set_path child telecom
pbap set_path child pb
```

- Get vCard list:

```text
pbap get_vcard_list pb 0
```

- Get one vCard:

```text
pbap get_vcard 0.vcf
```

- Get phonebook size:

```text
pbap get_size pb
```

- Abort transfer if needed:

```text
pbap abort
```

- Disconnect and deinitialize:

```text
pbap disconnect
pbap deinit
```

Expected behavior:
- INIT/DEINIT/CONNECT/DISCONNECT/SET_PHONEBOOK/GET_SIZE/ABORT events print status logs.
- CONNECT_CFM_EVT prints status, remote device address, supported repositories, and supported features.
- GET_PHONEBOOK_CFM_EVT parses the received body as vCard data and prints contact name, telephone number, call datetime when available, and total vCard count when the transfer finishes.
- GET_VCARD_LIST_CFM_EVT parses the received body as vCard listing data and prints card name and handle from entries such as:

```text
<card handle="0.vcf" name="Name"/>
```

- GET_VCARD_CFM_EVT parses the received body as one vCard and prints it similarly.

Notes:
- All commands except "pbap init", "pbap deinit", and "pbap connect" use the saved connected device address. If no device has been connected, the CLI reports:

```text
pbap: not connected. connect first.
```

Default object names used by the demo:

- get_phonebook/get_size default path is telecom/pb.vcf when "pb" is selected
- get_vcard_list default object is "pb"
- get_vcard default object is "0.vcf"
