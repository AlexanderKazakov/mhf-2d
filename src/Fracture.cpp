#include "Fracture.hpp"

Fracture::Fracture() {
}

Fracture::Fracture(int number, double x, double y, double beta, double h_length,
		int numOfElms, double pressure, double G, double nu, std::string pressureType):
		number(number), half_lengthOfBreaks(h_length), numOfBreaks(numOfElms),
		pressure(pressure), G(G), nu(nu), pressureType(pressureType) {
	
	breaks.push_back( Break(0, h_length, x, y, beta, G, nu, -pressure, 0) );
	numOfCalculatedBreaks = 1;
}

Fracture::~Fracture() {
}

Field Fracture::calculateImpactInPoint(const double &x, const double &y) const {
	std::vector<Break>::const_iterator break1 = breaks.begin();
	Field field;
	while (break1 != breaks.end()) {
		field += (*break1).calculateImpactInPoint(x, y);
		break1++;
	}
	return field;
}

void Fracture::calculate(std::vector<Fracture>::const_iterator firstFracture) {
	
	Field field;
	std::vector<Fracture>::const_iterator currentFracture = firstFracture;
	while (*currentFracture != *this) {
		field += (*currentFracture).calculateImpactInPoint(breaks.front().getCx(),
														breaks.front().getCy());
		currentFracture++;
	}
	//std::cout << field << std::endl;
	breaks.front().setExternalImpact(field);
	
	//	While the fracture is not stopped
	bool fractionIsStopped = false;
	while ( (numOfCalculatedBreaks <= numOfBreaks) && (!fractionIsStopped) ) {
		
		//	Calculate the existing configuration
		fractionIsStopped = calculateBreaks();
		
		//	Determine the direction of the fraction's growth
		double K1 = - breaks.front().getDn();
		double K2 = - breaks.front().getDs();
		double beta1 = breaks.front().getBeta() + calcAngleOfRotation(K1, K2);
		double x1 = breaks.front().getCx() - half_lengthOfBreaks * 
								( cos(beta1) + cos(breaks.front().getBeta()) );
		double y1 = breaks.front().getCy() - half_lengthOfBreaks *
								( sin(beta1) + sin(breaks.front().getBeta()) );
		breaks.insert(breaks.begin(), Break(-numOfCalculatedBreaks,
					half_lengthOfBreaks, x1, y1, beta1, G, nu, -pressure, 0));
		currentFracture = firstFracture;
		field.clear();
		while (*currentFracture != *this) {
			field += (*currentFracture).calculateImpactInPoint(x1, y1);
			currentFracture++;
		}
		breaks.front().setExternalImpact(field);
		//std::cout << K1 << "\t" << K2 << std::endl;


		K1 = - breaks.back().getDn();
		K2 = - breaks.back().getDs();
		beta1 = breaks.back().getBeta() + calcAngleOfRotation(K1, K2);
		x1 = breaks.back().getCx() + half_lengthOfBreaks *
				(cos(beta1) + cos(breaks.back().getBeta()));
		y1 = breaks.back().getCy() + half_lengthOfBreaks *
				(sin(beta1) + sin(breaks.back().getBeta()));
		breaks.push_back( Break(numOfCalculatedBreaks,
				half_lengthOfBreaks, x1, y1, beta1, G, nu, -pressure, 0));
		currentFracture = firstFracture;
		field.clear();
		while (*currentFracture != *this) {
			field += (*currentFracture).calculateImpactInPoint(x1, y1);
			currentFracture++;
		}
		breaks.back().setExternalImpact(field);
		
		numOfCalculatedBreaks++;
	}
	
//	std::vector<Break>::const_iterator brk = breaks.begin();
//	while (brk != breaks.end()) {
//		std::cout << *brk;
//		brk++;
//	}
	//std::cout << std::endl;
}

bool Fracture::calculateBreaks() {
	std::vector<Break>::iterator break1 = breaks.begin();
	std::vector<Break>::iterator break2 = breaks.begin();
	double Ass, Asn, Ans, Ann;
	int N = 2 * ( 2 * numOfCalculatedBreaks - 1 );
	gsl_matrix *A = gsl_matrix_alloc(N, N);
	gsl_vector *b = gsl_vector_alloc(N);
	gsl_vector *x = gsl_vector_alloc(N);
	
	int i = 0;
	while (break1 != breaks.end()) {
		break2 = breaks.begin();
		int j = 0;
		while (break2 != breaks.end()) {
			(*break1).calculateImpactOf(*break2, Ass, Asn, Ans, Ann);
			//std::cout << (*break1).getNumber() << "\t" << (*break2).getNumber() << std::endl;
			//std::cout << Ass << "\t" << Asn << "\t" << Ans << "\t" << Ann << "\t" << std::endl;
			gsl_matrix_set(A, i, j, Ass);
			gsl_matrix_set(A, i, j + 1, Asn);
			gsl_matrix_set(A, i + 1, j, Ans);
			gsl_matrix_set(A, i + 1, j + 1, Ann);
			j += 2;
			break2++;
		}
		//std::cout << std::endl;
		gsl_vector_set(b, i, (*break1).getBs());
		gsl_vector_set(b, i + 1, (*break1).getBn());
		i += 2;
		break1++;
		//std::cout << (*break1).getNumber() << "\t" << (*break2).getNumber() << std::endl;
	}
	//std::cout << std::endl;
	//std::cout << std::endl;

	gsl_permutation *p = gsl_permutation_alloc(N);
	int signum;	
	
//	for (int p = 0; p < N; p++) {
//		for (int q = 0; q < N; q++) {
//			std::cout << gsl_matrix_get(A, p, q) << "\t";
//		}
//		std::cout << "=\t" << gsl_vector_get(b, p) << std::endl;
//	}
//	std::cout << std::endl;
	
	gsl_linalg_LU_decomp(A, p, &signum);
	gsl_linalg_LU_solve(A, p, b, x);
	
	break1 = breaks.begin();
	i = 0;
	bool fractionIsStopped = false;
	while (break1 != breaks.end()) {
		double Dn = gsl_vector_get(x, i + 1);
		(*break1).setDs(gsl_vector_get(x, i));
		(*break1).setDn(Dn);
		if (Dn > 0) {
			(*break1).setDn(0);
			fractionIsStopped = true;
			std::cout << "Fraction is stoppped!" << std::endl;
		}		
		break1++;	
		i += 2;
	}
	return fractionIsStopped;
}

double Fracture::calcAngleOfRotation(const double& K1, const double& K2) const {
	double beta;
	beta = 2 * atan(- 2 * K2 / (K1 + sqrt(K1 * K1 + 8 * K2 * K2)));
	if ( (K1 + sqrt(K1 * K1 + 8 * K2 * K2)) == 0 ) {
		if (K2 != 0) {
			beta = - M_PI / 2;
		} else {
			std::cout << "Dn > 0!\n";
		}
	}
	return beta;
}

int Fracture::getNumOfPointsForPlot() const {
	return 2 * numOfCalculatedBreaks - 1;
}

void Fracture::getPointsForPlot(double* x, double* y) const {
	std::vector<Break>::const_iterator break1 = breaks.begin();
	int i = 0;
	while (break1 != breaks.end()) {
		x[i] = (*break1).getCx();
		y[i] = (*break1).getCy();
		i++;
		break1++;
	}
}

bool Fracture::operator==(const Fracture &other) const {
	return ( number == other.getNumber() );
}

bool Fracture::operator!=(const Fracture &other) const {
	return ( number != other.getNumber() );
}	

bool Fracture::operator<(const Fracture &other) const {
	return ( number < other.getNumber() );
}

int Fracture::getNumber() const {
	return number;
}