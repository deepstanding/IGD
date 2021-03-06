% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/IGDr.R
\name{get_ctgId}
\alias{get_ctgId}
\title{Function to get the contig id of a chromosome name}
\usage{
get_ctgId(igdr, chrm)
}
\arguments{
\item{igdr}{an igd database object (loaded)}

\item{chrm}{chromosome name ("chr1", "chrX", ...)}
}
\value{
ichr (0 if not exist)
}
\description{
Function to get the contig id of a chromosome name
}
\examples{
\dontrun{
ichr <- get_ctgId(igdr, "chrX")
}
}
