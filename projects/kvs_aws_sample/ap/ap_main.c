/**
 * kvs_aws_sample: AP entry for AWS KVS WebRTC P2P.
 * Master: send H264/H265/Opus from h264SampleFrames/h265SampleFrames/opusSampleFrames.
 * Viewer: receive and hand off to sampleVideoFrameHandler/sampleAudioFrameHandler in Common.c.
 * Role selected by CLI: run "kvs master" or "kvs viewer [channel_name]".
 *
 * TLS CA: enable CONFIG_KVS_GET_CA_FROM_ARRAY to use embedded PEM (kvs_embedded_ca_cert.c).
 * Sample media dirs remain on SD:
 * /sdcard/h264SampleFrames/, etc. SD is auto-mounted at SAMPLE_MEDIA_ROOT (/sdcard) at startup.
 */
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <stdint.h>
#include <stdlib.h>
#include "Samples.h"

#include "bk_partition.h"
#include "bk_posix.h"

extern int kvs_cli_init(void);

/** Mount SD card FATFS at SAMPLE_MEDIA_ROOT so sample frames (and optional files) are available. */
static void kvs_mount_sdcard(void)
{
	struct bk_fatfs_partition partition = {
		.part_type = FATFS_DEVICE,
		.part_dev.device_name = FATFS_DEV_SDCARD,
		.mount_path = SAMPLE_MEDIA_ROOT,
	};
	int ret = mount("SOURCE_NONE", partition.mount_path, "fatfs", 0, &partition);
	(void)ret; /* ignore: may already be mounted via CLI */
}


static void kvs_set_aws_credentials_env(void)
{
	setenv("AWS_ACCESS_KEY_ID", "YOUR_ACCESS_KEY_ID", 1);
	setenv("AWS_SECRET_ACCESS_KEY", "YOUR_SECRET_ACCESS_KEY", 1);
	/* Optional: setenv("AWS_DEFAULT_REGION", "us-east-1", 1); */
}

int main(void)
{
	bk_init();

	kvs_mount_sdcard();

	kvs_set_aws_credentials_env();

	kvs_cli_init();
	return 0;
}

