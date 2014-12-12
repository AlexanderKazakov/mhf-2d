#include "Stratum.hpp"

Stratum::Stratum() {
}

Stratum::~Stratum() {
}

void Stratum::addFracture(int number, double x, double y, double beta, double h_length,
		int numOfBreaks, double a, double b, double c, std::string pressureType,
		std::string tip, std::string rotation) {
	fractures.push_back(Fracture(this, number, h_length,
					numOfBreaks, a, b, c, pressureType, tip, rotation));
	fractures.back().allocateBreaks(x, y, beta);
}

void Stratum::setRheology(double _G, double _nu) {
	G = _G;		nu = _nu;
}

void Stratum::getRheology(double &_G, double &_nu) {
	_G = G;		_nu = nu;
}

void Stratum::setStresses(double _Sxx, double _Sxy, double _Syy) {
	Sxx = _Sxx;
	Sxy = _Sxy;
	Syy = _Syy;
}

void Stratum::setRanges(double _Xmin, double _Xmax, double _Ymin, double _Ymax) {
	Xmin = _Xmin;
	Xmax = _Xmax;
	Ymin = _Ymin;
	Ymax = _Ymax;		
}

void Stratum::reserve(int numberOfFractures) {
	fractures.reserve(numberOfFractures);
}

void Stratum::calculate() {
	currentFracture = fractures.begin();
	while (currentFracture != fractures.end()) {	
		currentFracture->calculate();
		currentFracture++;	
	}
}

Field Stratum::calculateImpactInPoint(const double& x, const double& y) {
	std::vector<Fracture>::iterator fracture = fractures.begin();
	Field field;
	while (fracture != currentFracture) {
		field += fracture->calculateImpactInPoint(x, y);
		fracture++;
	}
	field.Sxx += Sxx;
	field.Sxy += Sxy;
	field.Syy += Syy;
	return field;
}

void Stratum::visualize() {
	mglGraph gr = mglGraph(0, 1200, 800);
	gr.SetRanges(Xmin, Xmax, Ymin, Ymax);
	gr.Axis();

	drawFractures(gr);
	drawField(gr);
//	drawStressDirections(gr);
	
	gr.WriteFrame("fractures.png");
}

void Stratum::drawFractures(mglGraph& gr) {
	std::vector<Fracture>::iterator fracture = fractures.begin();
	while (fracture != fractures.end()) {
		int N = fracture->getNumOfBreaks() + 1;
		double *_x = new double[N];
		double *_y = new double[N];
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

void Stratum::drawField(mglGraph &gr) {
	int N = 104;
	mglData x(N);
	mglData y(N);
	mglData f(N, N);
	double maxF = 0;
	for (int i = 0; i < N; i += 1) {
		x.a[i] = (Xmax - Xmin) * i / (N-1) + Xmin;
		y.a[i] = (Ymax - Ymin) * i / (N-1) + Ymin;
	}
	for (int i = 0; i < N; i += 1)
		for (int j = 0; j < N; j += 1) {
			double _x = x.a[i];
			double _y = y.a[j];
			double _f = calculateImpactInPoint(_x, _y).Trace();
			maxF = (maxF > fabs(_f)) ? maxF : fabs(_f);
			f.a[i + N * j] = _f;
		}
	maxF = maxF / 10;
	for (int i = 0; i < N; i += 1)
		for (int j = 0; j < N; j += 1) {
			f.a[i + N * j] = f.a[i + N * j] / maxF;
		}
	gr.Dens(x, y, f);
	gr.Colorbar();
}

void Stratum::drawStressDirections(mglGraph &gr) {
	int N = 23;
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
			Field localField = calculateImpactInPoint(_x, _y);
			double Smax = localField.Smax() * 5;			
			ax.a[i + N * j] = Smax * cos(localField.directionOfMaxTensileStress());
			ay.a[i + N * j] = Smax * sin(localField.directionOfMaxTensileStress());
		}
	gr.Vect(x, y, ax, ay, "0");
	for (int i = 1; i < N; i += 1)
		for (int j = 1; j < N; j += 1) {
			ax.a[i + N * j] = - ax.a[i + N * j];
			ay.a[i + N * j] = - ay.a[i + N * j];
		}
	gr.Vect(x, y, ax, ay, "0");
}