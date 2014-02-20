#include "test.h"
#include "../src/mixer.h"
#include "../src/virtual.h"

TEST(test_effect_e9_retrig)
{
	xmp_context opaque;
	struct context_data *ctx;
	struct player_data *p;
	struct mixer_voice *vi;
	int voc;

	opaque = xmp_create_context();
	ctx = (struct context_data *)opaque;
	p = &ctx->p;

 	create_simple_module(ctx, 2, 2);
	new_event(ctx, 0, 0, 0, 60, 2, 40, 0x0e, 0x92, 0, 0);

	xmp_start_player(opaque, 44100, 0);

	/* Row 0 */

	/* frame 0 */
	xmp_play_frame(opaque);

	voc = map_channel(p, 0);
	fail_unless(voc >= 0, "virtual map");
	vi = &p->virt.voice_array[voc];

	fail_unless(vi->note == 59, "row 0 frame 0");
	fail_unless(vi->pos0 ==  0, "sample position frame 0");

	/* frame 1 */
	xmp_play_frame(opaque);
	fail_unless(vi->note == 59, "row 0 frame 1");
	fail_unless(vi->pos0 !=  0, "sample position frame 1");

	/* frame 2 */
	xmp_play_frame(opaque);
	fail_unless(vi->note == 59, "row 0 frame 2");
	fail_unless(vi->pos0 ==  0, "retrig frame 2");

	/* frame 3 */
	xmp_play_frame(opaque);
	fail_unless(vi->note == 59, "row 0 frame 3");
	fail_unless(vi->pos0 !=  0, "sample position frame 3");

	/* frame 4 */
	xmp_play_frame(opaque);
	fail_unless(vi->note == 59, "row 0 frame 4");
	fail_unless(vi->pos0 !=  0, "sample position frame 4");

	/* frame 5 */
	xmp_play_frame(opaque);
	fail_unless(vi->note == 59, "row 0 frame 5");
	fail_unless(vi->pos0 !=  0, "sample position frame 5");

}
END_TEST
