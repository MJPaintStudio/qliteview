// Keep the full locale set from the supplied ICU 71 data package. The upstream
// hand-written array only contains a small subset of locales.
alignas(16) unsigned char icudtlData[] = {
#embed "../../liteview/third_party/icu/common/icudtl.dat"
};
