SUBDIRS = mcmc

bin_PROGRAMS = reweighting \
	       initialize \
	       initialize_ind \
	       statMSA \
	       statMC_sigma_importance \
	       statMC_sigma \
	       autocorrelation \
	       compute_error_reparametrization \
	       compute_relaxation \
	       update_reparametrization \
	       update_learning_rate \
	       compute_energies

LIBS = -lm

DISTCLEANFILES = Makefile.in

reweighting_SOURCES = reweighting.c
initialize_SOURCES = initialize.c
initialize_ind_SOURCES = initialize_ind.c
statMSA_SOURCES = statistics_from_msa.c
statMC_sigma_importance_SOURCES = statistics_from_MC_2B_sigma_importance.c
statMC_sigma_SOURCES = statistics_from_MC_2B_sigma.c
autocorrelation_SOURCES = autocorrelation.c
compute_error_reparametrization_SOURCES = compute_error_reparametrization.c
compute_relaxation_SOURCES = compute_relaxation.c
update_reparametrization_SOURCES = update_reparametrization.c
update_learning_rate_SOURCES = update_learning_rate.c
compute_energies_SOURCES = compute_energies.c