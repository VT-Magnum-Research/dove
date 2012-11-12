# Read Data In
data = read.csv(file.open(), colClasses=c(rep("numeric", 11)))
names(data)

data$best_improv = ((data$opt - data$best) / data$opt)
data$median_improv = ((data$opt - data$median) / data$opt)

plot(data$parallelism, data$median_improv)
plot(data$crit_path, data$median_improv)
plot(data$time, data$median_improv)
plot(data$iterations, data$median_improv)
plot(data$ants, data$median_improv)
plot(data$cores, data$median_improv)