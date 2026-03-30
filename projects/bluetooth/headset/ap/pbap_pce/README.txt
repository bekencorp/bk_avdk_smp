PBAP PCE demo

Function:
This demo is used for PBAP PCE API functional verification.
It provides CLI commands to initialize PBAP PCE, connect/disconnect a remote device,
handle OBEX authentication, browse phonebook folders, get phonebook size, download
phonebook data, list vCard entries, get one vCard, and abort an ongoing operation.

Source files:
1. pbap_pce_demo.c
   Implements PBAP PCE callback registration, init/deinit, event queue/thread,
   and PBAP event handling.

2. pbap_pce_demo_cli.c
   Implements the "pbap" CLI command and command parameter parsing.

CLI usage:
  If running on the SMP branch, add the "ap_cmd" prefix before the command, for example:
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

Command description:
1. pbap init
   Create PBAP event queue/thread, register PBAP callback, and call bk_bt_pbap_pce_init().

2. pbap deinit
   Call bk_bt_pbap_pce_deinit(), unregister callback, and stop the PBAP event task.

3. pbap connect xx:xx:xx:xx:xx:xx [auth [pin [user_id]]]
   Connect to the remote PBAP server.
   If "auth" is present, optional pin and user_id are also passed during connect.
   The connected device address is saved and reused by following commands.

4. pbap disconnect
   Disconnect from the saved connected device.

5. pbap auth <pin> [user_id]
   Send OBEX authentication response for the saved connected device.
   When the peer actively initiates authentication, use this command to reply with the response.

6. pbap get_phonebook pb|mch|och|ich|cch|spd|fav [offset]
   Download one phonebook object or one call history object from the remote device.
   Object mapping:
   - pb  -> main phonebook, telecom/pb.vcf
   - mch -> missed call history, telecom/mch.vcf
   - och -> outgoing call history, telecom/och.vcf
   - ich -> incoming call history, telecom/ich.vcf
   - cch -> combined call history, telecom/cch.vcf
   - spd -> speed dial, telecom/spd.vcf
   - fav -> favorite contacts, telecom/fav.vcf
   Offset is optional and defaults to 0.

7. pbap set_path root|parent|child [folder_name]
   Change the current PBAP path on the remote device.
   Examples:
   - pbap set_path root
   - pbap set_path child telecom
   - pbap set_path child pb
   - pbap set_path child cch
   PBAP virtual folders structure:
    root
    |-- telecom
    |   |-- pb
    |   |-- ich
    |   |-- och
    |   |-- mch
    |   |-- cch
    |   |-- spd
    |   `-- fav
    `-- SIM1
        `-- telecom
            |-- pb
            |-- ich
            |-- och
            |-- mch
            `-- cch

8. pbap get_vcard_list [folder_name] [offset]
   Get the vCard listing of the current folder or of the given folder name.
   If folder_name is omitted, the default object name is "pb".
   Offset is optional and defaults to 0.

9. pbap get_vcard [vcard_name]
   Download one vCard object.
   If vcard_name is omitted, the default object name is "0.vcf".
   The vcard_name should be selected from the handle returned by pbap get_vcard_list, for example "0.vcf".

10. pbap get_size pb|mch|och|ich|cch|spd|fav
    Get remote phonebook size for the specified object.

11. pbap abort
    Abort the current PBAP operation for the saved connected device.

Typical test steps:
1. Initialize PBAP PCE:
   pbap init

2. Connect remote device:
   pbap connect xx:xx:xx:xx:xx:xx

3. If remote side requires OBEX authentication:
   pbap auth 123456

4. Get main phonebook:
   pbap get_phonebook pb 0

5. Get missed call history:
   pbap get_phonebook mch 0

6. Browse folders:
   pbap set_path root
   pbap set_path child telecom
   pbap set_path child pb

7. Get vCard list:
   pbap get_vcard_list pb 0

8. Get one vCard:
   pbap get_vcard 0.vcf

9. Get phonebook size:
   pbap get_size pb

10. Abort transfer if needed:
    pbap abort

11. Disconnect and deinitialize:
    pbap disconnect
    pbap deinit

Expected behavior:
1. INIT/DEINIT/CONNECT/DISCONNECT/SET_PHONEBOOK/GET_SIZE/ABORT events print status logs.

2. CONNECT_CFM_EVT prints:
   - status
   - remote device address
   - supported repositories
   - supported features

3. GET_PHONEBOOK_CFM_EVT:
   The received body is parsed as vCard data.
   The demo prints contact name, telephone number, and call datetime when available.
   When the transfer finishes, total vCard count is printed.

4. GET_VCARD_LIST_CFM_EVT:
   The received body is parsed as vCard listing data.
   The demo prints card name and handle from entries such as:
   <card handle="0.vcf" name="Name"/>

5. GET_VCARD_CFM_EVT:
   The received body is parsed as one vCard and printed similarly.

Notes:
1. All commands except "pbap init", "pbap deinit", and "pbap connect" use the saved
   connected device address. If no device has been connected, the CLI reports:
   "pbap: not connected. connect first."

2. Default object names used by the demo:
   - get_phonebook/get_size default path is telecom/pb.vcf when "pb" is selected
   - get_vcard_list default object is "pb"
   - get_vcard default object is "0.vcf"
