

struct markers
{
	int start1, end1, start2, end2, start3, end3;
};


void initialization();
void normalize(long double maxVal, long double minVal);
void removeDcShift(long double avg);
void fetchData();
void findEnergy();
markers setMarkers();
void trimSignal(markers m);
void step5();