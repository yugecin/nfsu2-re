static
void debug_custom_iterate_shops(int wparam)
{
	int i;
	struct CareerShop *shop;

	if (wparam == 121) { // y
		shop = *careerShops;
		for (i = 0; i < *numCareerShops; i++, shop++) {
			printf("SHOP %X:\n", shop->hash);
			printf("  markerHash: %X\n", shop->markerHash);
			printf("  74: %X\n", shop->field_74);
			printf("  9C: %d\n", shop->field_9C);
			printf("  stageIndex: %d\n", shop->stageIndex);
		}
	}

	DEBUG_WMCHAR_FUNC();
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_iterate_shops
