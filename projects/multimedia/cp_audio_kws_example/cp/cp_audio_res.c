#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/str.h>
#include "cli.h"
#include "bk_private/bk_cli.h"

#include "cp_audio_res.h"

typedef struct {
	const char *name;       /*!< resource name used in log lines */
	const char *owner;      /*!< holder name, NULL when free */
	uint32_t    refs;       /*!< references held by owner */
} cp_aud_res_t;

static cp_aud_res_t s_spk = { .name = "speaker" };
static cp_aud_res_t s_mic = { .name = "mic" };

static bk_err_t cp_aud_res_acquire(cp_aud_res_t *res, const char *owner)
{
	GLOBAL_INT_DECLARATION();

	if (!owner) {
		return BK_FAIL;
	}

	GLOBAL_INT_DISABLE();
	if (res->owner && os_strcmp(res->owner, owner) != 0) {
		const char *holder = res->owner;

		GLOBAL_INT_RESTORE();
		os_printf("cp_audio_res: %s busy (owner=%s), stop it first\r\n", res->name, holder);
		return BK_FAIL;
	}

	res->owner = owner;
	res->refs++;
	GLOBAL_INT_RESTORE();
	return BK_OK;
}

static void cp_aud_res_release(cp_aud_res_t *res, const char *owner)
{
	GLOBAL_INT_DECLARATION();

	if (!owner) {
		return;
	}

	GLOBAL_INT_DISABLE();
	if (!res->owner || os_strcmp(res->owner, owner) != 0) {
		const char *holder = res->owner;

		GLOBAL_INT_RESTORE();
		os_printf("cp_audio_res: %s release by %s but owner=%s\r\n",
				  res->name, owner, holder ? holder : "none");
		return;
	}

	if (--res->refs == 0) {
		res->owner = NULL;
	}
	GLOBAL_INT_RESTORE();
}

bk_err_t cp_aud_spk_acquire(const char *owner)
{
	return cp_aud_res_acquire(&s_spk, owner);
}

void cp_aud_spk_release(const char *owner)
{
	cp_aud_res_release(&s_spk, owner);
}

const char *cp_aud_spk_owner(void)
{
	return s_spk.owner;
}

bool cp_aud_spk_owned_by(const char *owner)
{
	return s_spk.owner && owner && os_strcmp(s_spk.owner, owner) == 0;
}

bk_err_t cp_aud_mic_acquire(const char *owner)
{
	return cp_aud_res_acquire(&s_mic, owner);
}

void cp_aud_mic_release(const char *owner)
{
	cp_aud_res_release(&s_mic, owner);
}

const char *cp_aud_mic_owner(void)
{
	return s_mic.owner;
}

void cp_aud_res_dump(void)
{
	os_printf("cp_audio_res: speaker owner=%s refs=%u\r\n",
			  s_spk.owner ? s_spk.owner : "none", s_spk.refs);
	os_printf("cp_audio_res: mic     owner=%s refs=%u\r\n",
			  s_mic.owner ? s_mic.owner : "none", s_mic.refs);
}

static void cli_cp_aud_res_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	(void)argc;
	(void)argv;

	cp_aud_res_dump();
}

static const struct cli_command s_cp_aud_res_commands[] = {
	{"cp_aud_res", "cp_aud_res - show speaker/mic owners", cli_cp_aud_res_cmd},
};

int cli_cp_aud_res_init(void)
{
	return cli_register_commands(s_cp_aud_res_commands,
								 sizeof(s_cp_aud_res_commands) / sizeof(s_cp_aud_res_commands[0]));
}
