package org.me.tulahack.service;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.me.tulahack.model.OptimizerRequest;
import org.me.tulahack.model.OptimizerResponse;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.nio.file.Path;
import java.util.concurrent.Semaphore;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

// TODO: исправить гонку потоков — использовать уникальные имена файлов через UUID
@Service
public class OptimizerClient {

    private final String optimizerPath;
    private final String inputFilePath;
    private final String outputFilePath;
    private final ObjectMapper objectMapper;
    private final Semaphore semaphore = new Semaphore(1);

    public OptimizerClient(@Value("${cpp.optimizer.path}") String optimizerPath,
                           @Value("${cpp.optimizer.input}") String inputFilePath,
                           @Value("${cpp.optimizer.output}") String outputFilePath,
                           ObjectMapper objectMapper) {
        this.optimizerPath = optimizerPath;
        this.inputFilePath = inputFilePath;
        this.outputFilePath = outputFilePath;
        this.objectMapper = objectMapper;
    }

    public OptimizerResponse optimize(OptimizerRequest request) {
        if (request.getPoints().isEmpty()) {
            return buildFallback(0);
        }

        try {
            semaphore.acquire();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return buildFallback(request.getPoints().size());
        }

        try {
            Path inputFile  = Path.of(inputFilePath);
            Path outputFile = Path.of(outputFilePath);

            objectMapper.writeValue(inputFile.toFile(), request);

            Process process = new ProcessBuilder(optimizerPath)
                    .inheritIO()
                    .start();

            process.waitFor();

            return objectMapper.readValue(outputFile.toFile(), OptimizerResponse.class);

        } catch (Exception e) {
            return buildFallback(request.getPoints().size());
        } finally {
            semaphore.release();
        }
    }

    private OptimizerResponse buildFallback(int size) {
        OptimizerResponse fallback = new OptimizerResponse();
        fallback.setOptimizedOrder(
                IntStream.range(0, size).boxed().collect(Collectors.toList())
        );
        fallback.setTotalDistance(0);
        fallback.setComputationMs(0L);
        return fallback;
    }
}
