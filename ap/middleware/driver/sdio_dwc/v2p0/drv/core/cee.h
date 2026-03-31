/******************************************************************************
Software Sample Terms and Conditions These Software Sample Terms and Conditions
("Terms") cover the Software you license from Synopsys, unless and until we
enter into new terms that expressly replace these Terms. If you use the Software
as an employee of or for the benefit of your company, your company will be the
licensee under these Terms. Accepting it you consent to these Terms on behalf of
yourself and the company on whose behalf you will use the Software. The
effective date of these Terms is the date that you click accepted them. If you
do not agree to these Terms or if you do not have the power and authority to
accept these Terms on behalf of your company, you may not use the Software and
Synopsys is unwilling to provide you with them.

1. The Software is not an item of Licensed Software or Licensed Product under
any end-user software license agreement with Synopsys or any supplement thereto.
Synopsys hereby grants to you a limited, personal, non-exclusive,
non-transferable, non-assignable, fully paid, royalty free, worldwide, perpetual
license to use the Software, and create modifications of the components of the
Software provided to you in source code format, solely for use with a Synopsys
DesignWare IP. All modifications of the Software are owned by Synopsys, and you
hereby irrevocably assign ownership of those modifications (and all intellectual
property rights therein) to Synopsys. However, you are under no obligation to
disclose any such modifications to Synopsys and the modifications are
automatically licensed to you as Software. The Software and all modifications
are the Confidential Information of Synopsys, and you agree not to distribute or
disclose them.

2. THE SOFTWARE IS PROVIDED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE HEREBY
DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THE SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

3. These Terms, which can be modified only by Synopsys in writing, shall be
governed by and construed under the laws of the State of California, USA,
without regard for its conflict of laws principles. You may not transfer or
assign your license rights to any other person in any manner (by assignment,
operation of law or otherwise) unless you have obtained written consent from
Synopsys. If you attempt to transfer or assign any of your license rights
without Synopsys's consent, the transfer or assignment will be ineffective,
null, and void. For purposes of this Section, a transfer or assignment of your
license rights will be deemed to have occurred (a) if a third party (or group of
third parties acting in concert) acquires beneficial ownership of fifty percent
(50%) or more of either your assets or of the stock or other equity interests
entitled to vote for your directors or equivalent managing authority, or (b) in
the event of a merger, consolidation or other business combination between you
and one or more third parties where your stockholders immediately before that
transaction own (directly or indirectly), after that transaction, less than
fifty percent (50%) of the stock or other equity interests entitled to vote for
the directors or equivalent managing authority of the surviving entity. These
Terms constitute the entire understanding and agreement between you and Synopsys
with respect to the subject matter hereof and supersedes and replaces all prior
and contemporaneous understandings and agreements, oral or written, express or
implied, regarding the same subject matter.
******************************************************************************/
/**
* \file		: cee.h
* \author	: ravibabu@synopsys.com
* \date		: 10-Nov-2019
* \brief	: cee.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _CEE_H_
#define _CEE_H_

#define MAX_NUM_CEE 4
#define MAX_PARAMS_LEN	256

struct io_req_t {
    uint8_t state;
    uint8_t status;
    uint8_t abort;
    uint8_t complete;
    uint8_t issued;
    uint8_t task_id;
    uint32_t *resp;
    uint32_t timeout;
    uint32_t repeat_cnt;
    uint32_t error;
    void *priv_data;
    void *req;
};

struct io_req_statistics {
    uint32_t tot_cnt;
    uint32_t pass_cnt;
    uint32_t fail_cnt;
};

struct cee_ops_t {
    void *priv_data;
    int (*submit_request)(void *priv_data, struct io_req_t *io_req);
    int (*prepare_io_req)(void *priv_data, struct io_req_t *io_req, void *param, uint32_t len);
    int (*release_io_req)(void *priv_data, struct io_req_t *io_req);
    int (*get_io_status)(void *priv_data, struct io_req_t *io_req);
};

struct cee_t {
    char name[8];
    uint8_t allocated;
    struct io_req_t io_req;
    struct cmd_param_s *cmd_param;
    uint8_t param[MAX_PARAMS_LEN];
    uint32_t param_len;
    struct cee_ops_t ops;
    struct io_req_statistics io_stat;
    uint8_t inp_req_queue;
};

/**
 * \brief cee_submit_request
 *     command execution engine (cee) submit request
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 */
uint8_t cee_submit_request(struct cee_t *cee, struct io_req_t *io_req);


/**
 * \brief cee_prepare_io_req
 *     command execution engine (cee) cmd preparation
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 * \param param: pointer to parameter list
 * \param len: paramerter length
 */
uint8_t cee_prepare_io_req(struct cee_t *cee, struct io_req_t *io_req,
                           void *param, uint32_t len);

/**
 * \brief cee_prepare_io_req
 *     release io_request
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 */
uint8_t cee_release_io_req(struct cee_t *cee, struct io_req_t *io_req);

/**
 * \brief get_io_status
 *     command execution engine (cee) get cmd status
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 */
uint8_t cee_get_io_status(struct cee_t *cee, struct io_req_t *io_req);

/**
 * \brief io_req_init
 * 	inititalize the request init
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 */
struct cee_t *cee_register(char *name, struct cee_ops_t *ops);

/**
 * \brief cee_init_
 *     command execution engine (cee) module init
 * \param level: debug level
 */
uint8_t cee_init(uint8_t level);

#endif
