#include "Stratum.hpp"

Stratum::Stratum() {
}

Stratum::~Stratum() {
}

void Stratum::addFracture(const Fracture& fracture) {
	fractures.push_back(fracture);
}

void Stratum::setRheology(double _G, double _nu) {
	G = _G;		nu = _nu;
}

void Stratum::sortFractures() {
	std::sort(fractures.begin(), fractures.end());
	currentFracture = fractures.begin();
}

int Stratum::calculateNextFracture() {
	if (fractures.end() == currentFracture)
		return 0;
	
	currentFracture->calculate(fractures.begin());
	currentFracture++;	
	return 1;
}

Field Stratum::calculateImpactInPoint(const double& x, const double& y) {
	std::vector<Fracture>::iterator fracture = fractures.begin();
	Field field;
	while (fracture != fractures.end()) {
		field += fracture->calculateImpactInPoint(x, y);
		fracture++;
	}
	return field;
}

void Stratum::visualize() {
	double Xmin = -22;
	double Xmax = 22;
	double Ymin = -22;
	double Ymax = 22;
	mglGraph gr = mglGraph(0, 1200, 800);
	gr.SetRanges(Xmin, Xmax, Ymin, Ymax);
	gr.Axis();

	drawFractures(gr);
	drawField(gr, Xmin, Xmax, Ymin, Ymax);
	drawDirections(gr, Xmin, Xmax, Ymin, Ymax);
	
	gr.WriteFrame("fractures.png");
}

void Stratum::drawFractures(mglGraph& gr) {
	std::vector<Fracture>::iterator fracture = fractures.begin();	
	while (fracture != fractures.end()) {
		int N = fracture->getNumOfBreaks() + 1;
		double *_x = new double[N];
		double *_y = new double[N];
		// TODO - remove _x and _y
		fracture->getPointsForPlot(_x, _y);
		mglData x;
		mglData y;
		x.Set(_x, N);
		y.Set(_y, N);
		gr.Plot(x, y, ".k");
		delete [] _x;
		delete [] _y;
		fracture++;
	}
}

void Stratum::drawField(mglGraph &gr, const double &Xmin, const double &Xmax,
									const double &Ymin, const double &Ymax) {
	int N = 101;
	mglData x(N);
	mglData y(N);
	mglData f(N, N);
	double maxF = 0;
	for (int i = 0; i < N; i += 1) {
		x.a[i] = (Xmax - Xmin) * i / N + Xmin;
		y.a[i] = (Ymax - Ymin) * i / N + Ymin;
	}
	for (int i = 0; i < N; i += 1)
		for (int j = 0; j < N; j += 1) {
			double _x = x.a[i];
			double _y = y.a[j];
			double _f = calculateImpactInPoint(_x, _y).Trace();
			maxF = (maxF > fabs(_f)) ? maxF : fabs(_f);
			f.a[i + N * j] = _f;
		}
	maxF = maxF / 0.9;
	for (int i = 0; i < N; i += 1)
		for (int j = 0; j < N; j += 1) {
			f.a[i + N * j] = f.a[i + N * j] / maxF;
		}
	gr.Dens(x, y, f);
	gr.Colorbar();
}

void Stratum::drawDirections(mglGraph &gr, const double &Xmin, const double &Xmax,
		const double &Ymin, const double &Ymax) {
	int N = 21;
	mglData x(N);
	mglData y(N);
	mglData ax(N, N);
	mglData ay(N, N);
	for (int i = 1; i < N; i += 1) {
		x.a[i] = (Xmax - Xmin) * i / N + Xmin;
		y.a[i] = (Ymax - Ymin) * i / N + Ymin;
	}
	for (int i = 1; i < N; i += 1)
		for (int j = 1; j < N; j += 1) {
			double _x = x.a[i];
			double _y = y.a[j];
			double Smax = calculateImpactInPoint(_x, _y).Smax() * 5;			
			ax.a[i + N * j] = Smax * cos(calculateImpactInPoint(_x, _y).directionOfMaxTensileStress());
			ay.a[i + N * j] = Smax * sin(calculateImpactInPoint(_x, _y).directionOfMaxTensileStress());
		}
	gr.Vect(x, y, ax, ay, "0");
	for (int i = 1; i < N; i += 1)
		for (int j = 1; j < N; j += 1) {
			ax.a[i + N * j] = - ax.a[i + N * j];
			ay.a[i + N * j] = - ay.a[i + N * j];
		}
	gr.Vect(x, y, ax, ay, "0");
}